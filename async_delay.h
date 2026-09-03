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
// - Idle tick cost is ~1/5 of a full scan when ASYNC_DELAY_OPT_BITMASK=1.
//   Optimization flags: ASYNC_DELAY_OPT_BITMASK (1), ASYNC_DELAY_OPT_MERGED_FLAGS (1),
//   ASYNC_DELAY_OPT_SPLIT_ARRAYS (0). See plans/003 for the full design.
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
// ============================================================
// Error Handling:
// ============================================================
//   - Missing ASYNC_DELAY_TICK_HZ -> #error at compile time
//   - Invalid TIMER_BITS value    -> #error at compile time
//   - MAX_SLOTS == 0              -> #error at compile time
//   - Invalid slot_id in elapsed/cancel -> safe, no crash
//   - No free slot in start       -> returns 0xFF
//   - tick_counter wrap           -> wrap-safe comparison (no early fire)
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

// Constraints on the opt combinations:
#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 8
#error "[async_delay] ASYNC_DELAY_OPT_BITMASK supports at most 8 slots (8-bit mask)."
#endif
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_OPT_SPLIT_ARRAYS requires ASYNC_DELAY_OPT_BITMASK=1."
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
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        _AD_FLAGS(i) = ASYNC_SLOT_FREE;
}

// Internal: common start for one-shot and periodic delays.
static unsigned char _async_delay_start_common(async_tick_t duration,
                                               async_delay_cb_t callback,
                                               unsigned char repeat)
{
    unsigned char i;
    async_tick_t now;

    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
#if ASYNC_DELAY_OPT_BITMASK
        if ((_async_active_mask & (1 << i)) == 0)   // bit clear => slot FREE
#else
        if (_AD_STATE(i) == ASYNC_SLOT_FREE)
#endif
        {
#if ASYNC_DELAY_TIMER_BITS >= 16
            // AVR is 8-bit: reading a 16/32-bit volatile variable is NOT atomic.
            // The ISR could increment the counter between byte loads, producing
            // a corrupted value (e.g. 0x00FF?0x0100 read as 0x01FF).
            // Disable interrupts briefly to get a consistent snapshot.
            #asm("cli")
            now = _async_tick_counter;
            #asm("sei")
#else
            now = _async_tick_counter;
#endif
            _AD_DUR(i)    = duration;
            _AD_TARGET(i) = now + duration;
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
#if ASYNC_DELAY_OPT_BITMASK
            // single-byte write, done LAST so the slot is fully set before it
            // becomes visible to the tick's mask walk. ISR never writes the
            // mask, so no cli is needed for this byte itself.
            _async_active_mask |= (1 << i);
#endif
            return i;
        }
    }
    return ASYNC_DELAY_NO_SLOT;
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
    if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
        return 0;

#if ASYNC_DELAY_OPT_BITMASK
    if (_async_active_mask & (1 << slot_id))      // still running -> not elapsed
        return 0;
#endif
    if (_AD_STATE(slot_id) == ASYNC_SLOT_EXPIRED)
    {
        _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
        return 1;
    }
    return 0;
}

// Cancel an active or expired delay. Frees the slot.
// Safe to call with invalid slot_id - does nothing silently.
static void async_delay_cancel(unsigned char slot_id)
{
    if (slot_id < ASYNC_DELAY_MAX_SLOTS)
    {
#if ASYNC_DELAY_OPT_BITMASK
        _async_active_mask &= (unsigned char)~(1 << slot_id);
#endif
        _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
    }
}

// ISR-context: process slot i that has been confirmed expired.
static void _async_delay_expire_slot(unsigned char i)
{
#if ASYNC_DELAY_CALLBACK_RESCHEDULE
    if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
    {
        // Periodic: re-arm FIRST so the slot stays ACTIVE for its next
        // cycle; the callback then runs with this slot still busy, so a
        // self-reschedule from the callback lands in a DIFFERENT slot.
        _AD_TARGET(i) += _AD_DUR(i);
        _AD_CB(i)(i);
        // state stays ACTIVE
    }
    else
    {
        // One-shot: free the slot BEFORE the callback so a
        // self-reschedule can reuse this very slot.
        if (_AD_CB(i) != (void *)0)
        {
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
            _AD_CB(i)(i);
        }
        else
        {
            // Polling mode: mark EXPIRED, user checks with elapsed()
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
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
        _AD_CB(i)(i);
        _AD_TARGET(i) += _AD_DUR(i);
        // state stays ACTIVE
    }
    else
    {
        // One-shot: fire callback (if any) then free the slot.
        if (_AD_CB(i) != (void *)0)
        {
            _AD_CB(i)(i);
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
        }
        else
        {
            // Polling mode: mark EXPIRED, user checks with elapsed()
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_EXPIRED;
        }
    }
#endif
}

// MUST be called from timer ISR at exactly ASYNC_DELAY_TICK_HZ rate.
// Increments tick counter, checks all active slots, fires callbacks.
// WARNING: Callbacks execute in ISR context - keep them very short!
static void async_delay_tick(void)
{
#if ASYNC_DELAY_OPT_BITMASK
    unsigned char m;
    _async_tick_counter++;              // MUST increment even when idle
    m = _async_active_mask;
    if (m == 0)
        return;                         // idle tick: ~12-16 cycles total

#if ASYNC_DELAY_MAX_SLOTS > 4
    // 5..8 slots: scan at most 8 bits, skipping cleared ones. Idle still returns
    // above; this only runs when at least one slot is ACTIVE.
    {
        unsigned char i;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if (m & (1 << i))
            {
                if ((async_tick_t)(_async_tick_counter - _AD_TARGET(i)) < _ASYNC_HALF_RANGE)
                    _async_delay_expire_slot(i);
            }
        }
    }
#else
    // 1..4 slots: direct low-bit chain - no loop, no table.
    while (m)
    {
        unsigned char i;
        if (m & 1)
            i = 0;
        else if (m & 2)
            i = 1;
        else if (m & 4)
            i = 2;
        else
            i = 3;
        m &= (unsigned char)~(1 << i);
        if ((async_tick_t)(_async_tick_counter - _AD_TARGET(i)) < _ASYNC_HALF_RANGE)
            _async_delay_expire_slot(i);
    }
#endif
#else
    // Legacy: full linear scan (behavior identical to before when all flags 0).
    unsigned char i;
    _async_tick_counter++;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (_AD_STATE(i) == ASYNC_SLOT_ACTIVE)
        {
            // Wrap-safe "now is past target" test:
            // if (tick - target) < half_range, the delay has elapsed.
            // This stays correct across a 16/32-bit counter wrap, as long
            // as the delay is shorter than half the tick range.
            if ((async_tick_t)(_async_tick_counter - _AD_TARGET(i)) < _ASYNC_HALF_RANGE)
                _async_delay_expire_slot(i);
        }
    }
#endif
}

#endif
