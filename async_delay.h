// async_delay.h - Non-blocking delay library for AVR (ATmega8/16/32)
// CodeVisionAVR C Compiler compatible
// HEADER-ONLY: No separate .c file needed. Just #include this header.
//
// ============================================================
// Prerequisites:
// ============================================================
// 1. A hardware timer must be configured to interrupt at
//    (1 / ASYNC_DELAY_TICK_HZ) second intervals.
// 2. Inside that timer ISR, call async_delay_tick().
// 3. Call async_delay_init() once at the start of main().
//
// ============================================================
// Configuration macros (define BEFORE #include):
// ============================================================
//   ASYNC_DELAY_TIMER_BITS  - tick counter width: 8, 16 or 32
//                              default: 16
//   ASYNC_DELAY_MAX_SLOTS   - max concurrent delays
//                              default: 4
//   ASYNC_DELAY_TICK_HZ     - tick rate in Hz
//                              MUST be defined by user
//                              example: 1000 for 1ms ticks
//
//   Correctness flags (leave at 1 unless you are measuring their cost):
//   ASYNC_DELAY_FIX_USED_MASK      - default 1. Track ALLOCATED slots
//                              separately, so an EXPIRED-but-unpolled slot is
//                              not handed out by start().
//   ASYNC_DELAY_FIX_ATOMIC_MASK    - default 1. SREG-preserving critical
//                              sections around the shared masks.
//
//   Speed flags (identical observable behavior):
//   ASYNC_DELAY_OPT_BITMASK        - default 1. Visit only ACTIVE slots.
//   ASYNC_DELAY_OPT_MERGED_FLAGS   - default 1. state+repeat in one byte.
//   ASYNC_DELAY_OPT_UNROLL_TICK    - default 1. Compile-time slot indices.
//   ASYNC_DELAY_OPT_NEXT_TARGET    - default 1. O(1) earliest-target gate.
//   ASYNC_DELAY_OPT_SPLIT_ARRAYS   - default 0. Parallel arrays instead of a
//                              struct (opt-in; measure before adopting).
//
//   Behavior-changing (opt in consciously):
//   ASYNC_DELAY_DEFERRED_CALLBACKS - default 0. Callbacks run from
//                              async_delay_poll() in MAIN context, not the ISR.
//   ASYNC_DELAY_CALLBACK_RESCHEDULE - default 1. Slot is made non-ACTIVE
//                              before its callback runs.
//
// ============================================================
// Accuracy & limits:
// ============================================================
// - Library logic error: at most +-1 tick (e.g. +-1ms at 1ms tick).
// - Real accuracy is set by YOUR clock source, not the library:
//     * internal RC 8MHz : about +-1% .. +-3%
//     * external crystal : about +-0.005%
// - Max reliable delay = half the tick range:
//     8-bit  tick: 128 ticks
//     16-bit tick: 32767 ticks   (32.7s at 1ms tick)
//     32-bit tick: ~2^31 ticks   (24.8 days at 1ms tick)
//   A delay longer than half the range is unreliable (wrap ambiguity).
// - Tick cost is O(1), not O(slots): an idle tick returns right after the
//   active-slot mask test, and a tick where nothing is due yet returns after
//   ONE wrap-safe compare against the cached earliest target - no matter how
//   many slots are running.
//   Optimization flags: ASYNC_DELAY_OPT_BITMASK (1), ASYNC_DELAY_OPT_MERGED_FLAGS (1),
//   ASYNC_DELAY_OPT_SPLIT_ARRAYS (0), ASYNC_DELAY_OPT_UNROLL_TICK (1),
//   ASYNC_DELAY_OPT_NEXT_TARGET (1). Correctness flags:
//   ASYNC_DELAY_FIX_USED_MASK (1), ASYNC_DELAY_FIX_ATOMIC_MASK (1).
//   Opt-in: ASYNC_DELAY_DEFERRED_CALLBACKS (0).
//   See plans/003 and plans/004 for the full design.
//
// ============================================================
// Quick Start:
// ============================================================
//   // 1. Define config before include
//   #define ASYNC_DELAY_TICK_HZ  1000
//   #include <async_delay.h>
//
//   // 2. In timer ISR (example uses Timer2 on ATmega8):
//   interrupt [TIM2_COMP] void timer2_isr(void) {
//       async_delay_tick();
//   }
//
//   // 3. In main():
//   async_delay_init();
//
//   // One-shot callback mode (fires ONCE):
//   void on_timeout(unsigned char id) { /* keep short! */ }
//   async_delay_start(500, on_timeout);
//
//   // Periodic callback mode (fires every 500ms, auto re-arms):
//   async_delay_start_periodic(500, on_timeout);
//
//   // Polling mode (one-shot, you check elapsed()):
//   unsigned char id = async_delay_start(500, (void *)0);
//   if (async_delay_elapsed(id)) { /* time is up */ }
//
//   // Cancel:
//   async_delay_cancel(id);
//
//   // OPTIONAL (ASYNC_DELAY_DEFERRED_CALLBACKS=1): callbacks are NOT run from
//   // the ISR. Drain them from the main loop instead - then a callback may use
//   // delay_ms, the LCD, anything. If you never call poll(), they never fire.
//   while (1) { async_delay_poll(); /* ... rest of your loop ... */ }
// ============================================================
// Error Handling:
// ============================================================
//   - Missing ASYNC_DELAY_TICK_HZ -> #error at compile time
//   - Invalid TIMER_BITS value    -> #error at compile time
//   - MAX_SLOTS == 0              -> #error at compile time
//   - Invalid slot_id in elapsed/cancel -> safe, no crash
//   - No free slot in start       -> returns 0xFF
//   - tick_counter wrap           -> wrap-safe comparison (no early fire)
//   - An EXPIRED polling slot stays ALLOCATED until you call elapsed() on it,
//     so start() returns 0xFF rather than handing out a slot whose owner has
//     not polled it yet. Forgetting elapsed() leaks the slot - by design, it
//     is the alternative to silently losing your timer.
//   - start()/cancel()/elapsed() save and restore SREG around the shared
//     state instead of a bare sei, so calling them from inside your own
//     critical section (or before sei in main) does not enable interrupts
//     behind your back.
//   - Self-rescheduling (start() inside a callback): one-shot callbacks are
//     freed before firing, so a new start() inside them works. Periodic
//     callbacks re-arm before firing; do NOT re-start the SAME periodic id
//     from inside its callback (it would create a second timer).
// ============================================================

#ifndef _ASYNC_DELAY_INCLUDED_
#define _ASYNC_DELAY_INCLUDED_

// ---------- Configuration validation ----------

#ifndef ASYNC_DELAY_TICK_HZ
#error "[async_delay] ASYNC_DELAY_TICK_HZ must be defined before including this header. Example: #define ASYNC_DELAY_TICK_HZ 1000"
#endif

#if ASYNC_DELAY_TICK_HZ == 0
#error "[async_delay] ASYNC_DELAY_TICK_HZ must be greater than zero."
#endif

#ifndef ASYNC_DELAY_TIMER_BITS
#define ASYNC_DELAY_TIMER_BITS 16
#endif

#if ASYNC_DELAY_TIMER_BITS != 8 && ASYNC_DELAY_TIMER_BITS != 16 && ASYNC_DELAY_TIMER_BITS != 32
#error "[async_delay] ASYNC_DELAY_TIMER_BITS must be 8, 16, or 32."
#endif

#ifndef ASYNC_DELAY_MAX_SLOTS
#define ASYNC_DELAY_MAX_SLOTS 4
#endif

#if ASYNC_DELAY_MAX_SLOTS == 0
#error "[async_delay] ASYNC_DELAY_MAX_SLOTS must be at least 1."
#endif

#if ASYNC_DELAY_MAX_SLOTS > 254
#error "[async_delay] ASYNC_DELAY_MAX_SLOTS cannot exceed 254 (slot id 0xFF is reserved as error code)."
#endif

// Order of operations when a callback fires in async_delay_tick():
//   1 = make the slot non-ACTIVE BEFORE calling the callback, so a callback
//       that starts a new delay (self-reschedule) grabs a free slot cleanly.
//   0 = legacy order (callback first, slot freed/re-armed after) — can return
//       0xFF from start() inside a one-shot callback, or double-fire on periodic.
#ifndef ASYNC_DELAY_CALLBACK_RESCHEDULE
#define ASYNC_DELAY_CALLBACK_RESCHEDULE 1
#endif

// ============ Optimization flags ============
// ASYNC_DELAY_OPT_BITMASK   : 1 = active-slot bitmask; async_delay_tick()
//                             visits ONLY the slots whose bit is set in
//                             _async_active_mask (idle tick ~12 cycles vs ~100).
#ifndef ASYNC_DELAY_OPT_BITMASK
#define ASYNC_DELAY_OPT_BITMASK 1
#endif

// ASYNC_DELAY_OPT_MERGED_FLAGS : 1 = merge state + repeat into one flags byte
//                             per slot (saves 1 byte/slot + one load in tick).
#ifndef ASYNC_DELAY_OPT_MERGED_FLAGS
#define ASYNC_DELAY_OPT_MERGED_FLAGS 1
#endif

// ASYNC_DELAY_OPT_SPLIT_ARRAYS : 1 = split the slot struct into parallel arrays
//                             (target/duration/callback/flags). Slightly faster
//                             AVR addressing (no struct offsets). Opt-in.
#ifndef ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define ASYNC_DELAY_OPT_SPLIT_ARRAYS 0
#endif

// ASYNC_DELAY_FIX_USED_MASK : 1 = track ALLOCATED slots in a second byte
//                             (_async_used_mask) instead of inferring "free"
//                             from the ACTIVE bitmask. Without this, a polling
//                             slot that already expired (bit clear, state
//                             EXPIRED) looks free to start(), which then
//                             overwrites it and the caller's elapsed() never
//                             reports. Costs 1 byte RAM; fixes a real bug.
#ifndef ASYNC_DELAY_FIX_USED_MASK
#define ASYNC_DELAY_FIX_USED_MASK 1
#endif

// ASYNC_DELAY_FIX_ATOMIC_MASK : 1 = guard every main-context mask
//                             read-modify-write with an SREG-preserving
//                             critical section. `mask |= bit` is LDS/OR/STS on
//                             AVR, so an ISR bit-clear landing mid-sequence is
//                             lost and leaves a dead slot marked ACTIVE (one
//                             spurious callback). Also stops start() from
//                             force-enabling interrupts via a bare sei.
//                             Costs ~2 cycles per call; fixes a real race.
#ifndef ASYNC_DELAY_FIX_ATOMIC_MASK
#define ASYNC_DELAY_FIX_ATOMIC_MASK 1
#endif

// ASYNC_DELAY_OPT_UNROLL_TICK : 1 = the tick tests each slot through a macro
//                             with a COMPILE-TIME index, so (1<<n), ~(1<<n)
//                             and &slot[n] all fold to literals. Removes the
//                             __LSLW12 shift-loop call, the i*7 MUL and the
//                             __GETW1P call from the hot path (~72 -> ~13
//                             cycles per active slot). Costs Flash.
#ifndef ASYNC_DELAY_OPT_UNROLL_TICK
#define ASYNC_DELAY_OPT_UNROLL_TICK 1
#endif

// ASYNC_DELAY_OPT_NEXT_TARGET : 1 = cache the earliest ACTIVE target in
//                             _async_next_target. A tick where nothing is due
//                             exits after ONE wrap-safe compare instead of
//                             visiting every active slot: O(1) instead of
//                             O(active). The O(N) minimum is recomputed only
//                             when a slot actually expires, or on
//                             start()/cancel(). Costs 2/4 bytes RAM.
#ifndef ASYNC_DELAY_OPT_NEXT_TARGET
#define ASYNC_DELAY_OPT_NEXT_TARGET 1
#endif

// ASYNC_DELAY_DEFERRED_CALLBACKS : 1 = the ISR does NOT call callbacks; it sets
//                             a bit in _async_pending_mask and the app drains
//                             them from async_delay_poll() in MAIN context.
//                             Without an ICALL the compiler stops spilling 11
//                             registers per tick (idle ~86 -> ~32 cycles), and
//                             callbacks may use delay_ms/LCD freely.
//                             CHANGES BEHAVIOR: callback latency becomes one
//                             main-loop iteration, you MUST call
//                             async_delay_poll(), and two expiries of the same
//                             slot before one poll collapse into one call.
//                             Off by default - opt in consciously.
#ifndef ASYNC_DELAY_DEFERRED_CALLBACKS
#define ASYNC_DELAY_DEFERRED_CALLBACKS 0
#endif

// Constraints on the opt combinations:
#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 8
#error "[async_delay] ASYNC_DELAY_OPT_BITMASK supports at most 8 slots (8-bit mask)."
#endif
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_OPT_SPLIT_ARRAYS requires ASYNC_DELAY_OPT_BITMASK=1."
#endif
#if ASYNC_DELAY_OPT_UNROLL_TICK && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_OPT_UNROLL_TICK requires ASYNC_DELAY_OPT_BITMASK=1."
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_OPT_NEXT_TARGET requires ASYNC_DELAY_OPT_BITMASK=1."
#endif
#if ASYNC_DELAY_FIX_USED_MASK && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_FIX_USED_MASK requires ASYNC_DELAY_OPT_BITMASK=1 (the legacy path uses the state byte and is already correct)."
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET && ASYNC_DELAY_TIMER_BITS >= 16 && !ASYNC_DELAY_FIX_ATOMIC_MASK
#error "[async_delay] ASYNC_DELAY_OPT_NEXT_TARGET with TIMER_BITS>=16 requires ASYNC_DELAY_FIX_ATOMIC_MASK=1 (the ISR reads a multi-byte _async_next_target)."
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS && ASYNC_DELAY_MAX_SLOTS > 8
#error "[async_delay] ASYNC_DELAY_DEFERRED_CALLBACKS supports at most 8 slots (8-bit pending mask)."
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_DEFERRED_CALLBACKS requires ASYNC_DELAY_OPT_BITMASK=1."
#endif

// ---------- Tick type based on TIMER_BITS ----------
#if ASYNC_DELAY_TIMER_BITS == 8
    typedef unsigned char async_tick_t;
#elif ASYNC_DELAY_TIMER_BITS == 16
    typedef unsigned int async_tick_t;
#elif ASYNC_DELAY_TIMER_BITS == 32
    typedef unsigned long async_tick_t;
#endif

// Callback signature: receives the slot id that expired
typedef void (*async_delay_cb_t)(unsigned char slot_id);

// Slot states
#define ASYNC_SLOT_FREE     0
#define ASYNC_SLOT_ACTIVE   1
#define ASYNC_SLOT_EXPIRED  2

// Error return value for async_delay_start when no slot available
#define ASYNC_DELAY_NO_SLOT 0xFF

// ---------- Internal data (static to avoid multiple-definition) ----------

#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define ASYNC_FLAG_REPEAT  0x04   // flags bit 2 = periodic
typedef struct {
    async_tick_t     target;    // tick when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    flags;     // [1:0]=state (FREE/ACTIVE/EXPIRED), [2]=repeat
} _async_slot_t;
#else
typedef struct {
    async_tick_t     target;    // tick when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    state;     // FREE / ACTIVE / EXPIRED
    unsigned char    repeat;    // 1 = periodic (auto re-arm), 0 = one-shot
} _async_slot_t;
#endif

#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
static async_tick_t     _async_target[ASYNC_DELAY_MAX_SLOTS];
static async_tick_t     _async_duration[ASYNC_DELAY_MAX_SLOTS];
static async_delay_cb_t _async_callback[ASYNC_DELAY_MAX_SLOTS];
static unsigned char    _async_flags[ASYNC_DELAY_MAX_SLOTS];
#if !ASYNC_DELAY_OPT_MERGED_FLAGS
static unsigned char    _async_repeat[ASYNC_DELAY_MAX_SLOTS];
#endif
#else
static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
#endif

static volatile async_tick_t _async_tick_counter;
#if ASYNC_DELAY_OPT_BITMASK
static volatile unsigned char _async_active_mask;   // bit n = slot n ACTIVE
#endif
#if ASYNC_DELAY_FIX_USED_MASK
// bit n = slot n ALLOCATED (ACTIVE *or* EXPIRED-waiting-for-elapsed).
// Needed because the ACTIVE mask alone cannot tell FREE from EXPIRED, so
// start() would hand out a slot whose owner has not polled it yet.
// Invariant: (_async_active_mask & ~_async_used_mask) == 0
static volatile unsigned char _async_used_mask;
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET
// Earliest target among ACTIVE slots (wrap-safe "earliest"). Meaningless while
// _async_active_mask == 0 - the mask test in the tick runs first.
// It is safe for this to be EARLIER than the true minimum (one wasted slot
// walk); it must NEVER be later, or a slot fires late.
static volatile async_tick_t _async_next_target;
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS
// bit n = slot n expired and still owes its callback to async_delay_poll().
static volatile unsigned char _async_pending_mask;
#endif

// Uniform per-slot access (works for both layouts)
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define _AD_TARGET(i)  (_async_target[i])
#define _AD_DUR(i)     (_async_duration[i])
#define _AD_CB(i)      (_async_callback[i])
#define _AD_FLAGS(i)   (_async_flags[i])
#else
#define _AD_TARGET(i)  (_async_slots[i].target)
#define _AD_DUR(i)     (_async_slots[i].duration)
#define _AD_CB(i)      (_async_slots[i].callback)
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_FLAGS(i)   (_async_slots[i].flags)
#else
#define _AD_FLAGS(i)   (_async_slots[i].state)
#endif
#endif
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_STATE(i)   ((unsigned char)(_AD_FLAGS(i) & 0x03))
#define _AD_REPEAT(i)  ((_AD_FLAGS(i) & ASYNC_FLAG_REPEAT) != 0)
#else
#define _AD_STATE(i)   (_AD_FLAGS(i))
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define _AD_REPEAT(i)  (_async_repeat[i])
#else
#define _AD_REPEAT(i)  (_async_slots[i].repeat)
#endif
#endif

// Half the tick range - compile-time constant (was recomputed every tick).
#define _ASYNC_HALF_RANGE ((async_tick_t)(~((async_tick_t)0) >> 1))

// Wrap-safe "time `t` has been reached at time `now`".
// Equivalently "t is not later than now". Bit-identical to the original
// (counter - target) < half test; kept in ONE place so the tick, the
// next-target minimum and start() cannot drift apart.
// Correct across a counter wrap as long as every delay < half the range.
#define _ASYNC_REACHED(now, t) \
    ((async_tick_t)((async_tick_t)(now) - (async_tick_t)(t)) < _ASYNC_HALF_RANGE)

// ---------- Critical section (MAIN context only) ----------
// The masks and _async_next_target are shared with the ISR. On AVR
// `mask |= bit` is LDS/OR/STS, so an ISR bit-clear landing mid-sequence is
// lost. Main-context code therefore brackets every such update.
//
// SREG is saved/restored rather than using a bare `sei`: a bare `sei` turns
// interrupts ON even if the caller had them off (start() called before `sei`
// in main(), or from inside another critical section).
//
// The `#asm("cli")` is written literally at each call site instead of being
// hidden in a macro body - CodeVisionAVR's #asm does not reliably survive
// macro expansion.
#if ASYNC_DELAY_FIX_ATOMIC_MASK
#define _ASYNC_CRIT_DECL     unsigned char _ad_sreg;
#define _ASYNC_SAVE_SREG()   _ad_sreg = SREG
#define _ASYNC_REST_SREG()   SREG = _ad_sreg
#else
#define _ASYNC_CRIT_DECL
#define _ASYNC_SAVE_SREG()
#define _ASYNC_REST_SREG()
#endif

// ---------- Public API Implementation ----------

// Initialize all slots to FREE and reset tick counter.
// MUST be called once at startup before any other async_delay function.
static void async_delay_init(void)
{
    unsigned char i;
    _async_tick_counter = 0;
#if ASYNC_DELAY_OPT_BITMASK
    _async_active_mask = 0;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
    _async_used_mask = 0;
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    _async_pending_mask = 0;
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET
    _async_next_target = 0;     // value irrelevant while active_mask == 0
#endif
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        _AD_FLAGS(i) = ASYNC_SLOT_FREE;
}

#if ASYNC_DELAY_OPT_NEXT_TARGET
// Recompute _async_next_target = earliest target among ACTIVE slots.
// O(MAX_SLOTS), but it only runs on the rare ticks where a slot actually
// expired, plus in cancel(). start() uses a single compare instead.
// CALLER MUST have interrupts disabled (or be in ISR context).
static void _async_recompute_next(void)
{
    unsigned char i, m, first;
    async_tick_t best;

    m = _async_active_mask;
    if (m == 0)
        return;                     // value unused while nothing is active

    best  = 0;
    first = 1;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (m & (unsigned char)(1 << i))
        {
            // _ASYNC_REACHED(best, t) == "t is not later than best"
            if (first || _ASYNC_REACHED(best, _AD_TARGET(i)))
            {
                best  = _AD_TARGET(i);
                first = 0;
            }
        }
    }
    _async_next_target = best;
}
#endif

// Internal: common start for one-shot and periodic delays.
static unsigned char _async_delay_start_common(async_tick_t duration,
                                               async_delay_cb_t callback,
                                               unsigned char repeat)
{
    unsigned char i;
#if ASYNC_DELAY_OPT_BITMASK || ASYNC_DELAY_FIX_USED_MASK
    // NOTE: not named `bit` - that is a CodeVisionAVR type specifier keyword.
    unsigned char slotbit;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
    unsigned char u;
#endif
    async_tick_t now, tgt;
    _ASYNC_CRIT_DECL

    // ---- Find a slot that is not ALLOCATED ----
#if ASYNC_DELAY_FIX_USED_MASK
    // Single-byte read is atomic on AVR. An EXPIRED slot keeps its used bit,
    // so it is NOT handed out until the owner calls elapsed().
    u = _async_used_mask;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        if ((u & (unsigned char)(1 << i)) == 0)
            break;
#else
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
#if ASYNC_DELAY_OPT_BITMASK
        if ((_async_active_mask & (unsigned char)(1 << i)) == 0)
            break;
#else
        if (_AD_STATE(i) == ASYNC_SLOT_FREE)
            break;
#endif
#endif
    if (i >= ASYNC_DELAY_MAX_SLOTS)
        return ASYNC_DELAY_NO_SLOT;

#if ASYNC_DELAY_OPT_BITMASK || ASYNC_DELAY_FIX_USED_MASK
    slotbit = (unsigned char)(1 << i);
#endif

    // ---- One critical section for the whole shared-state update ----
    // AVR is 8-bit: reading a 16/32-bit volatile variable is NOT atomic. The
    // ISR could increment the counter between byte loads, producing a
    // corrupted value (e.g. 0x00FF/0x0100 read as 0x01FF). The same window
    // also protects the mask read-modify-writes and _async_next_target from
    // an interleaved ISR update.
    _ASYNC_SAVE_SREG();
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
    #asm("cli")
#endif
    now = _async_tick_counter;
    tgt = (async_tick_t)(now + duration);

    _AD_DUR(i)    = duration;
    _AD_TARGET(i) = tgt;
    _AD_CB(i)     = callback;
#if ASYNC_DELAY_OPT_MERGED_FLAGS
    // state + repeat in one byte
    _AD_FLAGS(i)  = (unsigned char)(ASYNC_SLOT_ACTIVE |
                                    (repeat ? ASYNC_FLAG_REPEAT : 0));
#else
    // separate state byte (and repeat, only in the struct layout)
    _AD_FLAGS(i)  = ASYNC_SLOT_ACTIVE;
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
    _async_repeat[i] = repeat;      // split layout: own repeat array
#else
    _async_slots[i].repeat = repeat;  // struct layout
#endif
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET
    // Keep the earliest target. Must be evaluated BEFORE the active bit is
    // set, so `_async_active_mask == 0` still means "stored value is stale".
    if (_async_active_mask == 0 || _ASYNC_REACHED(_async_next_target, tgt))
        _async_next_target = tgt;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
    _async_used_mask |= slotbit;
#endif
#if ASYNC_DELAY_OPT_BITMASK
    // Done LAST so the slot is fully built before it becomes visible to the
    // tick's mask walk.
    _async_active_mask |= slotbit;
#endif
    _ASYNC_REST_SREG();
#if !ASYNC_DELAY_FIX_ATOMIC_MASK && ASYNC_DELAY_TIMER_BITS >= 16
    #asm("sei")
#endif
    return i;
}

// Start a ONE-SHOT non-blocking delay (fires once).
// duration : number of ticks (e.g. 500 for 500ms if TICK_HZ=1000)
//            duration=0 means immediate expire on next tick
// callback : called from ISR context on expiry, or (void *)0 for polling
// Returns  : slot id on success, ASYNC_DELAY_NO_SLOT if all slots busy
static unsigned char async_delay_start(async_tick_t duration, async_delay_cb_t callback)
{
    return _async_delay_start_common(duration, callback, 0);
}

// Start a PERIODIC non-blocking delay (fires every `duration` ticks
// and automatically re-arms itself until cancelled).
// callback must NOT be NULL (a periodic delay without a callback has
// no effect - it degrades to one-shot polling mode).
static unsigned char async_delay_start_periodic(async_tick_t duration,
                                                async_delay_cb_t callback)
{
    return _async_delay_start_common(duration, callback, 1);
}

// Check if a polling-mode delay has expired.
// Returns 1 if expired (and frees the slot), 0 otherwise.
// Safe to call with invalid slot_id - returns 0 silently.
static unsigned char async_delay_elapsed(unsigned char slot_id)
{
    // Declared at function scope: CodeVisionAVR follows C89 block rules, so a
    // declaration cannot appear after a statement.
#if ASYNC_DELAY_FIX_USED_MASK
    _ASYNC_CRIT_DECL
#endif

    if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
        return 0;

#if ASYNC_DELAY_OPT_BITMASK
    if (_async_active_mask & (unsigned char)(1 << slot_id))
        return 0;                                 // still running
#endif
    if (_AD_STATE(slot_id) == ASYNC_SLOT_EXPIRED)
    {
#if ASYNC_DELAY_FIX_USED_MASK
        // Releasing the allocation is what finally makes the slot reusable.
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
        #asm("cli")
#endif
        _async_used_mask &= (unsigned char)~(1 << slot_id);
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_REST_SREG();
#endif
#endif
        _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
        return 1;
    }
    return 0;
}

// Cancel an active or expired delay. Frees the slot.
// Safe to call with invalid slot_id - does nothing silently.
static void async_delay_cancel(unsigned char slot_id)
{
#if ASYNC_DELAY_OPT_BITMASK
    unsigned char clr;
#if ASYNC_DELAY_FIX_ATOMIC_MASK
    _ASYNC_CRIT_DECL
#endif
#endif

    if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
        return;

#if ASYNC_DELAY_OPT_BITMASK
    clr = (unsigned char)~(1 << slot_id);

#if ASYNC_DELAY_FIX_ATOMIC_MASK
    _ASYNC_SAVE_SREG();
    #asm("cli")
#endif
    _async_active_mask &= clr;
#if ASYNC_DELAY_FIX_USED_MASK
    _async_used_mask   &= clr;
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    _async_pending_mask &= clr;      // drop a callback this slot still owed
#endif
    _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
#if ASYNC_DELAY_OPT_NEXT_TARGET
    // The cancelled slot may have been the minimum, so a full recompute is
    // required. Skipping it could leave _async_next_target LATER than the true
    // minimum, which makes a surviving slot fire late - never do that.
    _async_recompute_next();
#endif
#if ASYNC_DELAY_FIX_ATOMIC_MASK
    _ASYNC_REST_SREG();
#endif
#else
    _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
#endif
}

// ISR-context: process slot i that has been confirmed expired.
// clr = ~(1<<i), precomputed by the caller - a compile-time literal when the
// tick is unrolled, which is what keeps __LSLW12 out of the hot path.
// Interrupts are already off here (ISR context), so the mask read-modify-writes
// need no extra guard.
static void _async_delay_expire_slot(unsigned char i, unsigned char clr)
{
#if ASYNC_DELAY_CALLBACK_RESCHEDULE && !ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_cb_t cb;
#endif

#if ASYNC_DELAY_CALLBACK_RESCHEDULE
    if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
    {
        // Periodic: re-arm FIRST so the slot stays ACTIVE for its next
        // cycle; the callback then runs with this slot still busy, so a
        // self-reschedule from the callback lands in a DIFFERENT slot.
        _AD_TARGET(i) += _AD_DUR(i);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
        _async_pending_mask |= (unsigned char)~clr;
#else
        _AD_CB(i)(i);
#endif
        // state stays ACTIVE
    }
    else
    {
        // One-shot: free the slot BEFORE the callback so a
        // self-reschedule can reuse this very slot.
        if (_AD_CB(i) != (void *)0)
        {
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= clr;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
            _async_used_mask   &= clr;      // one-shot: fully deallocated
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
#if ASYNC_DELAY_DEFERRED_CALLBACKS
            // Do NOT clear _AD_CB here - async_delay_poll() still needs it.
            _async_pending_mask |= (unsigned char)~clr;
#else
            // Take a copy, null the slot's pointer, then call. A freed slot
            // with a live callback pointer would be an ICALL target if its
            // mask bit were ever wrongly set.
            cb = _AD_CB(i);
            _AD_CB(i) = (void *)0;
            cb(i);
#endif
        }
        else
        {
            // Polling mode: mark EXPIRED, user checks with elapsed().
            // The slot STAYS allocated in _async_used_mask until then.
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= clr;
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_EXPIRED;
        }
    }
#else
    // Legacy ordering (flag 001 = 0): callback fires while slot still ACTIVE.
    if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
    {
        // Periodic: fire callback, then re-arm using target +=
        // duration so timing stays steady even if a tick is late.
#if ASYNC_DELAY_DEFERRED_CALLBACKS
        _async_pending_mask |= (unsigned char)~clr;
#else
        _AD_CB(i)(i);
#endif
        _AD_TARGET(i) += _AD_DUR(i);
        // state stays ACTIVE
    }
    else
    {
        // One-shot: fire callback (if any) then free the slot.
        if (_AD_CB(i) != (void *)0)
        {
#if ASYNC_DELAY_DEFERRED_CALLBACKS
            _async_pending_mask |= (unsigned char)~clr;
#else
            _AD_CB(i)(i);
#endif
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= clr;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
            _async_used_mask   &= clr;
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
        }
        else
        {
            // Polling mode: mark EXPIRED, user checks with elapsed()
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= clr;
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_EXPIRED;
        }
    }
#endif
}

#if ASYNC_DELAY_OPT_UNROLL_TICK
// Set only when the next-target cache needs a recompute after this tick.
#if ASYNC_DELAY_OPT_NEXT_TARGET
#define _AD_MARK_FIRED()  _ad_fired = 1
#else
#define _AD_MARK_FIRED()
#endif

// Per-slot tick body. `n` MUST be a compile-time constant so that (1<<n),
// ~(1<<n) and &_async_slots[n] all fold to literals / absolute addresses.
// That is the entire point: with a runtime index CodeVisionAVR emits a call to
// __LSLW12 (a bit-at-a-time shift loop) for (1<<i), a MUL for the i*sizeof
// struct offset, and a __GETW1P call to load .target - roughly 72 cycles per
// slot instead of ~13.
// Do NOT introduce a runtime-indexed caller; it silently undoes all of it.
#define _AD_TICK_SLOT(n)                                                      \
    if (_ad_m & (unsigned char)(1 << (n)))                                    \
    {                                                                         \
        if (_ASYNC_REACHED(_ad_now, _AD_TARGET(n)))                           \
        {                                                                     \
            _async_delay_expire_slot((unsigned char)(n),                      \
                                     (unsigned char)~(1 << (n)));            \
            _AD_MARK_FIRED();                                                 \
        }                                                                     \
    }
#endif

// MUST be called from timer ISR at exactly ASYNC_DELAY_TICK_HZ rate.
// Increments tick counter, checks the active slots, fires callbacks.
// WARNING: Callbacks execute in ISR context - keep them very short!
//          (unless ASYNC_DELAY_DEFERRED_CALLBACKS=1, see async_delay_poll)
static void async_delay_tick(void)
{
#if ASYNC_DELAY_OPT_BITMASK
    async_tick_t  _ad_now;
    unsigned char _ad_m;
#if ASYNC_DELAY_OPT_NEXT_TARGET
    unsigned char _ad_fired;
#endif

    // Counter FIRST, always. Every early return below must not skip it or
    // delays drift whenever no slot happens to be active.
    _ad_now = (async_tick_t)(_async_tick_counter + 1);
    _async_tick_counter = _ad_now;

    _ad_m = _async_active_mask;
    if (_ad_m == 0)
        return;                         // idle tick

#if ASYNC_DELAY_OPT_NEXT_TARGET
    // Nothing is due yet: one wrap-safe compare and out, regardless of how
    // many slots are active. This is what makes the tick O(1).
    if (!_ASYNC_REACHED(_ad_now, _async_next_target))
        return;
    _ad_fired = 0;
#endif

#if ASYNC_DELAY_OPT_UNROLL_TICK
    _AD_TICK_SLOT(0)
#if ASYNC_DELAY_MAX_SLOTS > 1
    _AD_TICK_SLOT(1)
#endif
#if ASYNC_DELAY_MAX_SLOTS > 2
    _AD_TICK_SLOT(2)
#endif
#if ASYNC_DELAY_MAX_SLOTS > 3
    _AD_TICK_SLOT(3)
#endif
#if ASYNC_DELAY_MAX_SLOTS > 4
    _AD_TICK_SLOT(4)
#endif
#if ASYNC_DELAY_MAX_SLOTS > 5
    _AD_TICK_SLOT(5)
#endif
#if ASYNC_DELAY_MAX_SLOTS > 6
    _AD_TICK_SLOT(6)
#endif
#if ASYNC_DELAY_MAX_SLOTS > 7
    _AD_TICK_SLOT(7)
#endif
#else
    // Not unrolled: runtime bit scan. Kept so the unroll can be A/B measured.
    {
        unsigned char i;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if (_ad_m & (unsigned char)(1 << i))
            {
                if (_ASYNC_REACHED(_ad_now, _AD_TARGET(i)))
                {
                    _async_delay_expire_slot(i, (unsigned char)~(1 << i));
#if ASYNC_DELAY_OPT_NEXT_TARGET
                    _ad_fired = 1;
#endif
                }
            }
        }
    }
#endif

#if ASYNC_DELAY_OPT_NEXT_TARGET
    // Only on the rare ticks where something actually expired. Interrupts are
    // already off (ISR context), which is what _async_recompute_next requires.
    if (_ad_fired)
        _async_recompute_next();
#endif
#else
    // Legacy: full linear scan (behavior identical to before when all flags 0).
    unsigned char i;
    async_tick_t  _ad_now;
    _ad_now = (async_tick_t)(_async_tick_counter + 1);
    _async_tick_counter = _ad_now;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (_AD_STATE(i) == ASYNC_SLOT_ACTIVE)
        {
            // Wrap-safe "now is past target" test: if (tick - target) < half
            // the range, the delay has elapsed. Stays correct across a 16/32-bit
            // counter wrap as long as the delay is shorter than half the range.
            if (_ASYNC_REACHED(_ad_now, _AD_TARGET(i)))
                _async_delay_expire_slot(i, (unsigned char)~(1 << i));
        }
    }
#endif
}

#if ASYNC_DELAY_DEFERRED_CALLBACKS
// Drain the callbacks the ISR deferred. Call this from your MAIN loop.
// Callbacks invoked here run in MAIN context, NOT in the ISR - so they may use
// delay_ms, the LCD, or anything else. In exchange, callback latency is one
// main-loop iteration, and if a slot expires twice before one poll the two
// collapse into a single call (one bit is one bit).
static void async_delay_poll(void)
{
    unsigned char p, i;
#if ASYNC_DELAY_FIX_ATOMIC_MASK
    _ASYNC_CRIT_DECL
#endif

    _ASYNC_SAVE_SREG();
    #asm("cli")
    p = _async_pending_mask;
    _async_pending_mask = 0;
    _ASYNC_REST_SREG();
#if !ASYNC_DELAY_FIX_ATOMIC_MASK
    #asm("sei")     // no SREG copy to restore: fall back to enabling
#endif

    if (p == 0)
        return;

    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (p & (unsigned char)(1 << i))
        {
            if (_AD_CB(i) != (void *)0)
                _AD_CB(i)(i);
        }
    }
}
#endif

#endif
