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
//   ASYNC_DELAY_OPT_LUT_MASK       - default 1. Bit-mask lookup table to
//                              eliminate runtime __LSLW12 shift loops in AVR.
//   ASYNC_DELAY_OPT_LUT_ALLOC      - default 1. O(1) loop-free slot allocator
//                              table in static RAM/Flash.
//   ASYNC_DELAY_OPT_LUT_POPCOUNT   - default 1. O(1) table-driven active count.
//   ASYNC_DELAY_OPT_FAST_CANCEL    - default 1. Skip O(N) minimum recomputation
//                              in cancel() when canceled slot wasn't earliest.
//   ASYNC_DELAY_OPT_MERGED_FLAGS   - default 1. state+repeat in one byte.
//   ASYNC_DELAY_OPT_UNROLL_TICK    - default 1. Compile-time slot indices.
//   ASYNC_DELAY_OPT_NEXT_TARGET    - default 1. O(1) earliest-target gate.
//   ASYNC_DELAY_OPT_SPLIT_TICK     - default 1. Keep the tick's fast path
//                              local-free so CodeVisionAVR does not spill
//                              registers on idle ticks.
//   ASYNC_DELAY_OPT_SPLIT_ARRAYS   - default 0. Parallel arrays instead of a
//                              struct (opt-in; measure before adopting).
//
//   RAM & Flash footprint optimization flags:
//   ASYNC_DELAY_DISABLE_CALLBACKS  - default 0. Polling-only mode; removes
//                              callback function pointer from slots (saves 2B
//                              RAM/slot).
//   ASYNC_DELAY_DISABLE_PERIODIC   - default 0. One-shot only mode; removes
//                              stored duration from slots (saves 2B RAM/slot
//                              and removes periodic branches in the ISR).
//
//   Feature flags:
//   ASYNC_DELAY_FEATURE_RESTART    - default 1. Provide async_delay_restart()
//                              to retarget a delay without losing slot ID.
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

#ifdef __cplusplus
extern "C"
{
#endif

// Library version: number of the last plan that modified this header.
// The user compiles against a MIRROR copy (G:\Kaveh\CodeVsion\inc\async_delay.h),
// not this repository file - comparing this one constant tells whether the mirror
// is stale:
//
//     #if ASYNC_DELAY_VERSION != 6
//     #error "async_delay.h mirror is stale - copy the repo header over and rebuild"
//     #endif
//
// Bump by 1 in EVERY future plan that edits this header, and record the new value
// in plans/README.md and README.md. Costs zero Flash/RAM (preprocessor only).
#define ASYNC_DELAY_VERSION 5

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

// ASYNC_DELAY_OPT_LUT_MASK  : 1 = use fast bit-mask lookup table instead of
//                             runtime bit shifts (1 << slot_id). Eliminates
//                             __LSLW12 runtime shift loop calls on AVR.
#ifndef ASYNC_DELAY_OPT_LUT_MASK
#define ASYNC_DELAY_OPT_LUT_MASK 1
#endif

// ASYNC_DELAY_OPT_LUT_ALLOC : 1 = use table-driven O(1) loop-free first free
//                             slot search for slot allocation in start().
#ifndef ASYNC_DELAY_OPT_LUT_ALLOC
#define ASYNC_DELAY_OPT_LUT_ALLOC 1
#endif

// ASYNC_DELAY_OPT_LUT_POPCOUNT : 1 = use 16-entry nibble lookup table for
//                             instant O(1) active slot counting.
#ifndef ASYNC_DELAY_OPT_LUT_POPCOUNT
#define ASYNC_DELAY_OPT_LUT_POPCOUNT 1
#endif

// ASYNC_DELAY_DISABLE_CALLBACKS : 1 = Polling-only mode. Removes callback
//                             pointer from slot structs, saving 2 bytes RAM
//                             per slot and reducing ISR code size.
#ifndef ASYNC_DELAY_DISABLE_CALLBACKS
#define ASYNC_DELAY_DISABLE_CALLBACKS 0
#endif

// ASYNC_DELAY_DISABLE_PERIODIC : 1 = One-shot only mode. Removes duration
//                             storage from slot structs, saving 2 bytes RAM
//                             (or 4 with 32-bit tick) per slot and eliminating
//                             periodic re-arm branches in the ISR.
#ifndef ASYNC_DELAY_DISABLE_PERIODIC
#define ASYNC_DELAY_DISABLE_PERIODIC 0
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

// ASYNC_DELAY_OPT_UNROLL_RECOMPUTE : 1 = unrolls _async_recompute_next() with
//                             compile-time slot indices, eliminating the
//                             runtime loop counter, indexing MULs, and branch
//                             overhead when recalculating earliest deadline.
#ifndef ASYNC_DELAY_OPT_UNROLL_RECOMPUTE
#define ASYNC_DELAY_OPT_UNROLL_RECOMPUTE 1
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

// ASYNC_DELAY_OPT_FAST_CANCEL : 1 = O(1) cancel fast path. In cancel(), skip
//                             recomputing _async_next_target if the cancelled
//                             slot was not the earliest one or no active slots
//                             remain.
#ifndef ASYNC_DELAY_OPT_FAST_CANCEL
#define ASYNC_DELAY_OPT_FAST_CANCEL 1
#endif

// ASYNC_DELAY_FEATURE_RESTART : 1 = provides async_delay_restart() to retarget
//                             or restart an allocated slot without losing its ID.
#ifndef ASYNC_DELAY_FEATURE_RESTART
#define ASYNC_DELAY_FEATURE_RESTART 1
#endif

// ASYNC_DELAY_OPT_SPLIT_TICK : 1 = async_delay_tick() keeps NO locals and the
//                             slot walk lives in a separate function.
//                             CodeVisionAVR spills register locals with
//                             RCALL __SAVELOCR4 on entry and __LOADLOCR4 on
//                             exit - ~30 cycles paid on EVERY tick, including
//                             the idle ones that return three instructions
//                             later. Pushing the locals into the callee means
//                             that cost is only paid on ticks with real work.
//                             0 = the single-function shape (locals hoisted,
//                             walk inlined), for A/B measurement.
#ifndef ASYNC_DELAY_OPT_SPLIT_TICK
#define ASYNC_DELAY_OPT_SPLIT_TICK 1
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

// ASYNC_DELAY_FEATURE_SLEEP : 1 = Provide async_delay_sleep_idle() helper
//                             to safely put AVR MCU into IDLE sleep until
//                             next timer tick interrupt.
#ifndef ASYNC_DELAY_FEATURE_SLEEP
#define ASYNC_DELAY_FEATURE_SLEEP 0
#endif

// Constraints on the opt combinations:
#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 16
#error "[async_delay] ASYNC_DELAY_OPT_BITMASK supports at most 16 slots."
#endif
#if ASYNC_DELAY_MAX_SLOTS > 8 && !ASYNC_DELAY_FIX_ATOMIC_MASK
#error "[async_delay] ASYNC_DELAY_MAX_SLOTS > 8 requires ASYNC_DELAY_FIX_ATOMIC_MASK=1 (16-bit mask access is not atomic on 8-bit AVR)."
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
#if ASYNC_DELAY_DEFERRED_CALLBACKS && ASYNC_DELAY_MAX_SLOTS > 16
#error "[async_delay] ASYNC_DELAY_DEFERRED_CALLBACKS supports at most 16 slots."
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_DEFERRED_CALLBACKS requires ASYNC_DELAY_OPT_BITMASK=1."
#endif
#if ASYNC_DELAY_OPT_SPLIT_TICK && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_OPT_SPLIT_TICK requires ASYNC_DELAY_OPT_BITMASK=1."
#endif
#if ASYNC_DELAY_DISABLE_CALLBACKS && ASYNC_DELAY_DEFERRED_CALLBACKS
#error "[async_delay] ASYNC_DELAY_DEFERRED_CALLBACKS cannot be used when ASYNC_DELAY_DISABLE_CALLBACKS is enabled."
#endif

// ---------- Tick type based on TIMER_BITS ----------
#if ASYNC_DELAY_TIMER_BITS == 8
    typedef unsigned char async_tick_t;
#elif ASYNC_DELAY_TIMER_BITS == 16
typedef unsigned int async_tick_t;
#elif ASYNC_DELAY_TIMER_BITS == 32
typedef unsigned long async_tick_t;
#endif

// ---------- Mask type based on MAX_SLOTS ----------
#if ASYNC_DELAY_MAX_SLOTS <= 8
    typedef unsigned char async_mask_t;
#else
typedef unsigned int async_mask_t;
#endif

// ---------- Bit-mask lookup table for fast O(1) slot bit operations ----------
#if ASYNC_DELAY_OPT_LUT_MASK
#if ASYNC_DELAY_MAX_SLOTS <= 8
    static const unsigned char _async_slot_bit[8] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
#else
    static const unsigned int _async_slot_bit[16] = {
        0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};
#endif
#define _AD_SLOT_BIT(n) (_async_slot_bit[(n)])
#define _AD_SLOT_CLR(n) ((async_mask_t)~_async_slot_bit[(n)])
#else
#define _AD_SLOT_BIT(n) ((async_mask_t)(1 << (n)))
#define _AD_SLOT_CLR(n) ((async_mask_t) ~(1 << (n)))
#endif

// ---------- Lookup table for fast O(1) slot allocation (first free bit index) ----------
#if ASYNC_DELAY_OPT_LUT_ALLOC
    // Maps a 4-bit nibble of allocated mask to the first free slot index (0..3), or 4 if full (0x0F)
    static const unsigned char _async_first_free_nibble[16] = {
        0, // 0b0000 -> slot 0
        1, // 0b0001 -> slot 1
        0, // 0b0010 -> slot 0
        2, // 0b0011 -> slot 2
        0, // 0b0100 -> slot 0
        1, // 0b0101 -> slot 1
        0, // 0b0110 -> slot 0
        3, // 0b0111 -> slot 3
        0, // 0b1000 -> slot 0
        1, // 0b1001 -> slot 1
        0, // 0b1010 -> slot 0
        2, // 0b1011 -> slot 2
        0, // 0b1100 -> slot 0
        1, // 0b1101 -> slot 1
        0, // 0b1110 -> slot 0
        4  // 0b1111 -> full (no free slot in this nibble)
    };
#endif

// ---------- Lookup table for fast O(1) active slot count (nibble popcount) ----------
#if ASYNC_DELAY_OPT_LUT_POPCOUNT
    // Maps a 4-bit nibble to its population count (number of set bits)
    static const unsigned char _async_popcount_nibble[16] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
#endif

    // Callback signature: receives the slot id that expired
    typedef void (*async_delay_cb_t)(unsigned char slot_id);

// Slot states
#define ASYNC_SLOT_FREE 0
#define ASYNC_SLOT_ACTIVE 1
#define ASYNC_SLOT_EXPIRED 2

// Error return value for async_delay_start when no slot available
#define ASYNC_DELAY_NO_SLOT 0xFF

    // ---------- Internal data (static to avoid multiple-definition) ----------

#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define ASYNC_FLAG_REPEAT 0x04 // flags bit 2 = periodic
    typedef struct
    {
        async_tick_t target; // tick when this delay expires
#if !ASYNC_DELAY_DISABLE_PERIODIC
        async_tick_t duration; // stored for periodic re-arm
#endif
#if !ASYNC_DELAY_DISABLE_CALLBACKS
        async_delay_cb_t callback; // NULL = polling-only mode
#endif
        unsigned char flags; // [1:0]=state (FREE/ACTIVE/EXPIRED), [2]=repeat
    } _async_slot_t;
#else
typedef struct
{
    async_tick_t target; // tick when this delay expires
#if !ASYNC_DELAY_DISABLE_PERIODIC
    async_tick_t duration; // stored for periodic re-arm
#endif
#if !ASYNC_DELAY_DISABLE_CALLBACKS
    async_delay_cb_t callback; // NULL = polling-only mode
#endif
    unsigned char state; // FREE / ACTIVE / EXPIRED
#if !ASYNC_DELAY_DISABLE_PERIODIC
    unsigned char repeat; // 1 = periodic (auto re-arm), 0 = one-shot
#endif
} _async_slot_t;
#endif

#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
    static async_tick_t _async_target[ASYNC_DELAY_MAX_SLOTS];
#if !ASYNC_DELAY_DISABLE_PERIODIC
    static async_tick_t _async_duration[ASYNC_DELAY_MAX_SLOTS];
#endif
#if !ASYNC_DELAY_DISABLE_CALLBACKS
    static async_delay_cb_t _async_callback[ASYNC_DELAY_MAX_SLOTS];
#endif
    static unsigned char _async_flags[ASYNC_DELAY_MAX_SLOTS];
#if !ASYNC_DELAY_OPT_MERGED_FLAGS && !ASYNC_DELAY_DISABLE_PERIODIC
    static unsigned char _async_repeat[ASYNC_DELAY_MAX_SLOTS];
#endif
#else
static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
#endif

    static volatile async_tick_t _async_tick_counter;
#if ASYNC_DELAY_OPT_BITMASK
    static volatile async_mask_t _async_active_mask; // bit n = slot n ACTIVE
#endif
#if ASYNC_DELAY_FIX_USED_MASK
    // bit n = slot n ALLOCATED (ACTIVE *or* EXPIRED-waiting-for-elapsed).
    // Needed because the ACTIVE mask alone cannot tell FREE from EXPIRED, so
    // start() would hand out a slot whose owner has not polled it yet.
    // Invariant: (_async_active_mask & ~_async_used_mask) == 0
    static volatile async_mask_t _async_used_mask;
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
    static volatile async_mask_t _async_pending_mask;
#endif

// Uniform per-slot access (works for both layouts)
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define _AD_TARGET(i) (_async_target[i])
#if !ASYNC_DELAY_DISABLE_PERIODIC
#define _AD_DUR(i) (_async_duration[i])
#define _AD_SET_DUR(i, d) (_async_duration[i] = (d))
#else
#define _AD_DUR(i) 0
#define _AD_SET_DUR(i, d)
#endif
#if !ASYNC_DELAY_DISABLE_CALLBACKS
#define _AD_CB(i) (_async_callback[i])
#define _AD_SET_CB(i, cb) (_async_callback[i] = (cb))
#else
#define _AD_CB(i) ((void *)0)
#define _AD_SET_CB(i, cb)
#endif
#define _AD_FLAGS(i) (_async_flags[i])
#else
#define _AD_TARGET(i) (_async_slots[i].target)
#if !ASYNC_DELAY_DISABLE_PERIODIC
#define _AD_DUR(i) (_async_slots[i].duration)
#define _AD_SET_DUR(i, d) (_async_slots[i].duration = (d))
#else
#define _AD_DUR(i) 0
#define _AD_SET_DUR(i, d)
#endif
#if !ASYNC_DELAY_DISABLE_CALLBACKS
#define _AD_CB(i) (_async_slots[i].callback)
#define _AD_SET_CB(i, cb) (_async_slots[i].callback = (cb))
#else
#define _AD_CB(i) ((void *)0)
#define _AD_SET_CB(i, cb)
#endif
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_FLAGS(i) (_async_slots[i].flags)
#else
#define _AD_FLAGS(i) (_async_slots[i].state)
#endif
#endif
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_STATE(i) ((unsigned char)(_AD_FLAGS(i) & 0x03))
#else
#define _AD_STATE(i) (_AD_FLAGS(i))
#endif

#if ASYNC_DELAY_DISABLE_PERIODIC
#define _AD_REPEAT(i) 0
#else
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_REPEAT(i) ((_AD_FLAGS(i) & ASYNC_FLAG_REPEAT) != 0)
#else
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define _AD_REPEAT(i) (_async_repeat[i])
#else
#define _AD_REPEAT(i) (_async_slots[i].repeat)
#endif
#endif
#endif

// Half the tick range - compile-time constant (was recomputed every tick).
// LITERAL per-width constants, NOT `~((async_tick_t)0) >> 1`: with an 8-bit
// async_tick_t the ~0 operand promotes to signed int, ~0 -> -1, the
// arithmetic >> keeps -1, and the cast truncates it to 0xFF (255). A half
// range of 255 on a 256 counter strips _ASYNC_REACHED of its wrap margin -
// every delay fires ~immediately (found by the plan-006 host harness: T2
// got=1 want=40 under TIMER_BITS=8). Literals have no promotion step, so the
// value is exact on every compiler. 16-bit value unchanged (0x7FFF): the
// default build is bit-identical.
#if ASYNC_DELAY_TIMER_BITS == 8
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7F) // 127
#elif ASYNC_DELAY_TIMER_BITS == 16
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7FFF)     // 32767 (unchanged)
#else                                                /* 32 */
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7FFFFFFF) // 2147483647 (unchanged)
#endif

// Wrap-safe "time `t` has been reached at time `now`".
// Equivalently "t is not later than now". Bit-identical to the original
// (counter - target) < half test; kept in ONE place so the tick, the
// next-target minimum and start() cannot drift apart.
// Correct across a counter wrap as long as every delay < half the range.
#define _ASYNC_REACHED(now, t) \
    ((async_tick_t)((async_tick_t)(now) - (async_tick_t)(t)) < _ASYNC_HALF_RANGE)

// ---------- Critical section (MAIN context only) ----------
// ---------- Compiler & Platform Portability Layer ----------
// Supports CodeVisionAVR, AVR-GCC (Microchip Studio / Arduino),
// and Host GCC/Clang for native simulation and unit testing.
#if defined(__GNUC__) || defined(__clang__)
#if defined(__AVR__)
#include <avr/io.h>
#include <avr/interrupt.h>
#define _ASYNC_ASM_CLI() cli()
#else
// Host simulation (Linux / Windows / macOS testing)
#ifndef _ASYNC_HOST_SREG_DEFINED
#define _ASYNC_HOST_SREG_DEFINED
    static volatile unsigned char SREG = 0;
#endif
#define _ASYNC_ASM_CLI() ((void)0)
#endif
#endif

// The masks and _async_next_target are shared with the ISR. On AVR
// `mask |= bit` is LDS/OR/STS, so an ISR bit-clear landing mid-sequence is
// lost. Main-context code therefore brackets every such update.
//
// SREG is saved/restored rather than using a bare `sei`: a bare `sei` turns
// interrupts ON even if the caller had them off (start() called before `sei`
// in main(), or from inside another critical section).
//
// For CodeVisionAVR, #asm("cli") is executed directly, while for GCC/Clang
// _ASYNC_ASM_CLI() is invoked.
#if ASYNC_DELAY_FIX_ATOMIC_MASK
#define _ASYNC_CRIT_DECL unsigned char _ad_sreg;
#define _ASYNC_SAVE_SREG() _ad_sreg = SREG
#define _ASYNC_REST_SREG() SREG = _ad_sreg
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
        _async_next_target = 0; // value irrelevant while active_mask == 0
#endif
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
    }

#if ASYNC_DELAY_OPT_NEXT_TARGET
#if ASYNC_DELAY_OPT_UNROLL_RECOMPUTE
#define _AD_RECOMP_SLOT(n)                                \
    if (m & _AD_SLOT_BIT(n))                              \
    {                                                     \
        if (first || _ASYNC_REACHED(best, _AD_TARGET(n))) \
        {                                                 \
            best = _AD_TARGET(n);                         \
            first = 0;                                    \
        }                                                 \
        m &= (async_mask_t)~_AD_SLOT_BIT(n);              \
        if (m == 0)                                       \
        {                                                 \
            _async_next_target = best;                    \
            return;                                       \
        }                                                 \
    }

#if ASYNC_DELAY_MAX_SLOTS > 1
#define _AD_RECOMP_S1 _AD_RECOMP_SLOT(1)
#else
#define _AD_RECOMP_S1
#endif
#if ASYNC_DELAY_MAX_SLOTS > 2
#define _AD_RECOMP_S2 _AD_RECOMP_SLOT(2)
#else
#define _AD_RECOMP_S2
#endif
#if ASYNC_DELAY_MAX_SLOTS > 3
#define _AD_RECOMP_S3 _AD_RECOMP_SLOT(3)
#else
#define _AD_RECOMP_S3
#endif
#if ASYNC_DELAY_MAX_SLOTS > 4
#define _AD_RECOMP_S4 _AD_RECOMP_SLOT(4)
#else
#define _AD_RECOMP_S4
#endif
#if ASYNC_DELAY_MAX_SLOTS > 5
#define _AD_RECOMP_S5 _AD_RECOMP_SLOT(5)
#else
#define _AD_RECOMP_S5
#endif
#if ASYNC_DELAY_MAX_SLOTS > 6
#define _AD_RECOMP_S6 _AD_RECOMP_SLOT(6)
#else
#define _AD_RECOMP_S6
#endif
#if ASYNC_DELAY_MAX_SLOTS > 7
#define _AD_RECOMP_S7 _AD_RECOMP_SLOT(7)
#else
#define _AD_RECOMP_S7
#endif
#if ASYNC_DELAY_MAX_SLOTS > 8
#define _AD_RECOMP_S8 _AD_RECOMP_SLOT(8)
#else
#define _AD_RECOMP_S8
#endif
#if ASYNC_DELAY_MAX_SLOTS > 9
#define _AD_RECOMP_S9 _AD_RECOMP_SLOT(9)
#else
#define _AD_RECOMP_S9
#endif
#if ASYNC_DELAY_MAX_SLOTS > 10
#define _AD_RECOMP_S10 _AD_RECOMP_SLOT(10)
#else
#define _AD_RECOMP_S10
#endif
#if ASYNC_DELAY_MAX_SLOTS > 11
#define _AD_RECOMP_S11 _AD_RECOMP_SLOT(11)
#else
#define _AD_RECOMP_S11
#endif
#if ASYNC_DELAY_MAX_SLOTS > 12
#define _AD_RECOMP_S12 _AD_RECOMP_SLOT(12)
#else
#define _AD_RECOMP_S12
#endif
#if ASYNC_DELAY_MAX_SLOTS > 13
#define _AD_RECOMP_S13 _AD_RECOMP_SLOT(13)
#else
#define _AD_RECOMP_S13
#endif
#if ASYNC_DELAY_MAX_SLOTS > 14
#define _AD_RECOMP_S14 _AD_RECOMP_SLOT(14)
#else
#define _AD_RECOMP_S14
#endif
#if ASYNC_DELAY_MAX_SLOTS > 15
#define _AD_RECOMP_S15 _AD_RECOMP_SLOT(15)
#else
#define _AD_RECOMP_S15
#endif

#define _AD_RECOMP_SWEEP()                                             \
    _AD_RECOMP_SLOT(0)                                                 \
    _AD_RECOMP_S1 _AD_RECOMP_S2 _AD_RECOMP_S3 _AD_RECOMP_S4            \
        _AD_RECOMP_S5 _AD_RECOMP_S6 _AD_RECOMP_S7 _AD_RECOMP_S8        \
            _AD_RECOMP_S9 _AD_RECOMP_S10 _AD_RECOMP_S11 _AD_RECOMP_S12 \
                _AD_RECOMP_S13 _AD_RECOMP_S14 _AD_RECOMP_S15
#endif

    // Recompute _async_next_target = earliest target among ACTIVE slots.
    // O(MAX_SLOTS), but it only runs on the rare ticks where a slot actually
    // expired, plus in cancel(). start() uses a single compare instead.
    // CALLER MUST have interrupts disabled (or be in ISR context).
    static void _async_recompute_next(void)
    {
#if ASYNC_DELAY_OPT_UNROLL_RECOMPUTE
        unsigned char first;
        async_mask_t m;
        async_tick_t best;

        m = _async_active_mask;
        if (m == 0)
            return; // value unused while nothing is active

        best = 0;
        first = 1;
        _AD_RECOMP_SWEEP();
        (void)first;
        _async_next_target = best;
#else
        unsigned char i, first;
        async_mask_t m, slotbit;
        async_tick_t best;

        m = _async_active_mask;
        if (m == 0)
            return; // value unused while nothing is active

        best = 0;
        first = 1;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            slotbit = _AD_SLOT_BIT(i);
            if (m & slotbit)
            {
                // _ASYNC_REACHED(best, t) == "t is not later than best"
                if (first || _ASYNC_REACHED(best, _AD_TARGET(i)))
                {
                    best = _AD_TARGET(i);
                    first = 0;
                }
                m &= (async_mask_t)~slotbit;
                if (m == 0)
                    break; // early exit: all active slots visited
            }
        }
        (void)first;
        _async_next_target = best;
#endif
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
        async_mask_t slotbit;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
        async_mask_t u;
#endif
#if ASYNC_DELAY_OPT_BITMASK
        async_mask_t cur_active;
#endif
        async_tick_t now, tgt;
        _ASYNC_CRIT_DECL

        if (duration > _ASYNC_HALF_RANGE)
            return ASYNC_DELAY_NO_SLOT;

#if ASYNC_DELAY_DISABLE_CALLBACKS
        (void)callback;
#endif

        // ---- Find a slot that is not ALLOCATED ----
#if ASYNC_DELAY_FIX_USED_MASK
        // For <= 8 slots, single-byte read is atomic on AVR.
        // For > 8 slots, critical section guards multi-byte mask.
#if ASYNC_DELAY_MAX_SLOTS > 8 && ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
        u = _async_used_mask;
        _ASYNC_REST_SREG();
#else
        u = _async_used_mask;
#endif

#if ASYNC_DELAY_OPT_LUT_ALLOC
        // O(1) loop-free lookup using static 4-bit nibble table
        i = _async_first_free_nibble[u & 0x0F];
#if ASYNC_DELAY_MAX_SLOTS > 4
        if (i == 4)
            i = (unsigned char)(4 + _async_first_free_nibble[(u >> 4) & 0x0F]);
#endif
#if ASYNC_DELAY_MAX_SLOTS > 8
        if (i == 8)
            i = (unsigned char)(8 + _async_first_free_nibble[(u >> 8) & 0x0F]);
#endif
#if ASYNC_DELAY_MAX_SLOTS > 12
        if (i == 12)
            i = (unsigned char)(12 + _async_first_free_nibble[(u >> 12) & 0x0F]);
#endif
        if (i >= ASYNC_DELAY_MAX_SLOTS)
            return ASYNC_DELAY_NO_SLOT;
        slotbit = _AD_SLOT_BIT(i);
#else
        slotbit = 1;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if ((u & slotbit) == 0)
                break;
            slotbit <<= 1;
        }
        if (i >= ASYNC_DELAY_MAX_SLOTS)
            return ASYNC_DELAY_NO_SLOT;
#endif

#else
#if ASYNC_DELAY_OPT_BITMASK
#if ASYNC_DELAY_OPT_LUT_ALLOC
        cur_active = _async_active_mask;
        i = _async_first_free_nibble[cur_active & 0x0F];
#if ASYNC_DELAY_MAX_SLOTS > 4
        if (i == 4)
            i = (unsigned char)(4 + _async_first_free_nibble[(cur_active >> 4) & 0x0F]);
#endif
#if ASYNC_DELAY_MAX_SLOTS > 8
        if (i == 8)
            i = (unsigned char)(8 + _async_first_free_nibble[(cur_active >> 8) & 0x0F]);
#endif
#if ASYNC_DELAY_MAX_SLOTS > 12
        if (i == 12)
            i = (unsigned char)(12 + _async_first_free_nibble[(cur_active >> 12) & 0x0F]);
#endif
        if (i >= ASYNC_DELAY_MAX_SLOTS)
            return ASYNC_DELAY_NO_SLOT;
        slotbit = _AD_SLOT_BIT(i);
#else
        slotbit = 1;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if ((_async_active_mask & slotbit) == 0)
                break;
            slotbit <<= 1;
        }
        if (i >= ASYNC_DELAY_MAX_SLOTS)
            return ASYNC_DELAY_NO_SLOT;
#endif
#else
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if (_AD_STATE(i) == ASYNC_SLOT_FREE)
                break;
        }
        if (i >= ASYNC_DELAY_MAX_SLOTS)
            return ASYNC_DELAY_NO_SLOT;
#endif
#endif

        // ---- One critical section for the whole shared-state update ----
        // AVR is 8-bit: reading a 16/32-bit volatile variable is NOT atomic. The
        // ISR could increment the counter between byte loads, producing a
        // corrupted value (e.g. 0x00FF/0x0100 read as 0x01FF). The same window
        // also protects the mask read-modify-writes and _async_next_target from
        // an interleaved ISR update.
        _ASYNC_SAVE_SREG();
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif
        now = _async_tick_counter;
        tgt = (async_tick_t)(now + duration);

        _AD_SET_DUR(i, duration);
        _AD_TARGET(i) = tgt;
        _AD_SET_CB(i, callback);
#if ASYNC_DELAY_OPT_MERGED_FLAGS
        // state + repeat in one byte
        _AD_FLAGS(i) = (unsigned char)(ASYNC_SLOT_ACTIVE |
                                       (repeat ? ASYNC_FLAG_REPEAT : 0));
#else
    // separate state byte (and repeat, only in the struct layout)
    _AD_FLAGS(i) = ASYNC_SLOT_ACTIVE;
#if !ASYNC_DELAY_DISABLE_PERIODIC
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
    _async_repeat[i] = repeat; // split layout: own repeat array
#else
    _async_slots[i].repeat = repeat; // struct layout
#endif
#endif
#endif
#if ASYNC_DELAY_OPT_BITMASK
        cur_active = _async_active_mask;
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET
        // Keep the earliest target. Must be evaluated BEFORE the active bit is
        // set, so `cur_active == 0` still means "stored value is stale".
        // Caching cur_active eliminates redundant LDS and branch instructions.
        if (cur_active == 0 || _ASYNC_REACHED(_async_next_target, tgt))
            _async_next_target = tgt;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
        _async_used_mask |= slotbit;
#endif
#if ASYNC_DELAY_OPT_BITMASK
        // Done LAST so the slot is fully built before it becomes visible to the
        // tick's mask walk.
        _async_active_mask = (async_mask_t)(cur_active | slotbit);
#endif
        _ASYNC_REST_SREG();
#if !ASYNC_DELAY_FIX_ATOMIC_MASK && ASYNC_DELAY_TIMER_BITS >= 16
#if defined(__GNUC__) || defined(__clang__)
        sei();
#else
#asm("sei")
#endif
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
#if !ASYNC_DELAY_DISABLE_PERIODIC
    static unsigned char async_delay_start_periodic(async_tick_t duration,
                                                    async_delay_cb_t callback)
    {
        return _async_delay_start_common(duration, callback, 1);
    }
#endif

#if ASYNC_DELAY_FEATURE_RESTART
    // Restart or retarget an existing slot with a new duration.
    // The slot ID is preserved, and its target is updated relative to the current tick.
    // Can be called on ACTIVE or EXPIRED slots.
    // Returns 1 if successfully restarted, 0 if slot_id is invalid or FREE.
    static unsigned char async_delay_restart(unsigned char slot_id, async_tick_t new_duration)
    {
        async_mask_t slotbit;
#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_OPT_NEXT_TARGET
        async_mask_t cur_active;
#endif
        async_tick_t now, tgt;
        _ASYNC_CRIT_DECL

        if (slot_id >= ASYNC_DELAY_MAX_SLOTS || new_duration > _ASYNC_HALF_RANGE)
            return 0;

        slotbit = _AD_SLOT_BIT(slot_id);

        // Slot must be allocated (ACTIVE or EXPIRED) to be restarted
#if ASYNC_DELAY_FIX_USED_MASK
        if ((_async_used_mask & slotbit) == 0)
            return 0;
#else
        if (_AD_STATE(slot_id) == ASYNC_SLOT_FREE)
            return 0;
#endif

        _ASYNC_SAVE_SREG();
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif
        now = _async_tick_counter;
        tgt = (async_tick_t)(now + new_duration);

        _AD_SET_DUR(slot_id, new_duration);
        _AD_TARGET(slot_id) = tgt;

#if ASYNC_DELAY_OPT_MERGED_FLAGS
        _AD_FLAGS(slot_id) = (unsigned char)(ASYNC_SLOT_ACTIVE |
                                             (_AD_REPEAT(slot_id) ? ASYNC_FLAG_REPEAT : 0));
#else
        _AD_FLAGS(slot_id) = ASYNC_SLOT_ACTIVE;
#endif

#if ASYNC_DELAY_DEFERRED_CALLBACKS
        // Clear any unpolled pending callback from a previous expiry
        _async_pending_mask &= (async_mask_t)~slotbit;
#endif

#if ASYNC_DELAY_OPT_BITMASK
#if ASYNC_DELAY_OPT_NEXT_TARGET
        cur_active = _async_active_mask;
        if (cur_active == 0 || _ASYNC_REACHED(_async_next_target, tgt))
            _async_next_target = tgt;
        _async_active_mask = (async_mask_t)(cur_active | slotbit);
#else
        _async_active_mask |= slotbit;
#endif
#endif

        _ASYNC_REST_SREG();
#if !ASYNC_DELAY_FIX_ATOMIC_MASK && ASYNC_DELAY_TIMER_BITS >= 16
#if defined(__GNUC__) || defined(__clang__)
        sei();
#else
#asm("sei")
#endif
#endif
        return 1;
    }
#endif

    // Check if a polling-mode delay has expired.
    // Returns 1 if expired (and frees the slot), 0 otherwise.
    // Safe to call with invalid slot_id - returns 0 silently.
    static unsigned char async_delay_elapsed(unsigned char slot_id)
    {
        // Declared at function scope: CodeVisionAVR follows C89 block rules, so a
        // declaration cannot appear after a statement.
#if ASYNC_DELAY_OPT_BITMASK
        async_mask_t slotbit;
#endif
#if ASYNC_DELAY_FIX_USED_MASK || (ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 8)
        _ASYNC_CRIT_DECL
#endif

        if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
            return 0;

#if ASYNC_DELAY_OPT_BITMASK
        slotbit = _AD_SLOT_BIT(slot_id);
#if ASYNC_DELAY_MAX_SLOTS > 8 && ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
        if (_async_active_mask & slotbit)
        {
            _ASYNC_REST_SREG();
            return 0; // still running
        }
        _ASYNC_REST_SREG();
#else
        if (_async_active_mask & slotbit)
            return 0; // still running
#endif
#endif
        if (_AD_STATE(slot_id) == ASYNC_SLOT_EXPIRED)
        {
#if ASYNC_DELAY_FIX_USED_MASK
            // Releasing the allocation is what finally makes the slot reusable.
#if ASYNC_DELAY_FIX_ATOMIC_MASK
            _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
            _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif
            _async_used_mask &= (async_mask_t)~slotbit;
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
        async_mask_t clr;
        async_mask_t slotbit;
#if ASYNC_DELAY_OPT_FAST_CANCEL && ASYNC_DELAY_OPT_NEXT_TARGET
        async_tick_t old_target;
        unsigned char was_active;
#endif
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_CRIT_DECL
#endif
#endif

        if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
            return;

#if ASYNC_DELAY_OPT_BITMASK
        slotbit = _AD_SLOT_BIT(slot_id);
        clr = (async_mask_t)~slotbit;

#if ASYNC_DELAY_OPT_FAST_CANCEL && ASYNC_DELAY_OPT_NEXT_TARGET
        old_target = _AD_TARGET(slot_id);
#endif

#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif

#if ASYNC_DELAY_OPT_FAST_CANCEL && ASYNC_DELAY_OPT_NEXT_TARGET
        was_active = ((_async_active_mask & slotbit) != 0);
#endif

        _async_active_mask &= clr;
#if ASYNC_DELAY_FIX_USED_MASK
        _async_used_mask &= clr;
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS
        _async_pending_mask &= clr; // drop a callback this slot still owed
#endif
        _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
#if ASYNC_DELAY_OPT_NEXT_TARGET
#if ASYNC_DELAY_OPT_FAST_CANCEL
        // The cancelled slot may have been the minimum. If it was NOT active, or
        // was not the minimum, or no slots remain active, no recomputation is needed!
        if (was_active && old_target == _async_next_target && _async_active_mask != 0)
            _async_recompute_next();
#else
        _async_recompute_next();
#endif
#endif
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_REST_SREG();
#endif
#else
    _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
#endif
    }

    // ---------- Utility & Power-Saving APIs ----------

    // Check if a slot is currently active and counting down.
    // Returns 1 if active, 0 if free, expired, or slot_id is invalid.
    static unsigned char async_delay_is_active(unsigned char slot_id)
    {
        if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
            return 0;
#if ASYNC_DELAY_OPT_BITMASK
        return (_async_active_mask & _AD_SLOT_BIT(slot_id)) != 0;
#else
    return (_AD_STATE(slot_id) == ASYNC_SLOT_ACTIVE);
#endif
    }

    // Return the total count of currently active delays.
    static unsigned char async_delay_active_count(void)
    {
        unsigned char count;
#if ASYNC_DELAY_OPT_BITMASK
        async_mask_t m;
#if ASYNC_DELAY_MAX_SLOTS > 8 && ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_CRIT_DECL
#endif
#endif
#if !ASYNC_DELAY_OPT_BITMASK
        unsigned char i;
#endif

#if ASYNC_DELAY_OPT_BITMASK
#if ASYNC_DELAY_MAX_SLOTS > 8 && ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
        m = _async_active_mask;
        _ASYNC_REST_SREG();
#else
        m = _async_active_mask;
#endif

#if ASYNC_DELAY_OPT_LUT_POPCOUNT
        count = _async_popcount_nibble[m & 0x0F];
#if ASYNC_DELAY_MAX_SLOTS > 4
        count = (unsigned char)(count + _async_popcount_nibble[(m >> 4) & 0x0F]);
#endif
#if ASYNC_DELAY_MAX_SLOTS > 8
        count = (unsigned char)(count + _async_popcount_nibble[(m >> 8) & 0x0F]);
#endif
#if ASYNC_DELAY_MAX_SLOTS > 12
        count = (unsigned char)(count + _async_popcount_nibble[(m >> 12) & 0x0F]);
#endif
#else
        count = 0;
        while (m)
        {
            count = (unsigned char)(count + (m & 1));
            m >>= 1;
        }
#endif
#else
        count = 0;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if (_AD_STATE(i) == ASYNC_SLOT_ACTIVE)
                count++;
        }
#endif
        return count;
    }

    // Calculate remaining ticks until slot_id expires.
    // Returns 0 if slot_id is invalid, not active, or already reached.
    static async_tick_t async_delay_remaining(unsigned char slot_id)
    {
        async_tick_t now, tgt;
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_CRIT_DECL
#endif

        if (!async_delay_is_active(slot_id))
            return 0;

#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif
        now = _async_tick_counter;
        tgt = _AD_TARGET(slot_id);
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_REST_SREG();
#endif

        if (_ASYNC_REACHED(now, tgt))
            return 0;
        return (async_tick_t)(tgt - now);
    }

    // Calculate ticks remaining until the earliest active delay expires.
    // Returns 0 if no delays are active or if the earliest delay is already due.
    // Designed for MCU power saving (e.g. Sleep / IDLE mode or prescaler tuning).
    static async_tick_t async_delay_ticks_until_next(void)
    {
        async_tick_t now, next_tgt;
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_CRIT_DECL
#endif
#if !ASYNC_DELAY_OPT_NEXT_TARGET
        unsigned char i, first = 1;
#endif

#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif

#if ASYNC_DELAY_OPT_BITMASK
        if (_async_active_mask == 0)
        {
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
            _ASYNC_REST_SREG();
#endif
            return 0;
        }
#endif
        now = _async_tick_counter;
#if ASYNC_DELAY_OPT_NEXT_TARGET
        next_tgt = _async_next_target;
#else
    next_tgt = 0;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (_AD_STATE(i) == ASYNC_SLOT_ACTIVE)
        {
            if (first || _ASYNC_REACHED(next_tgt, _AD_TARGET(i)))
            {
                next_tgt = _AD_TARGET(i);
                first = 0;
            }
        }
    }
    if (first)
    {
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_REST_SREG();
#endif
        return 0;
    }
#endif
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_REST_SREG();
#endif

        if (_ASYNC_REACHED(now, next_tgt))
            return 0;
        return (async_tick_t)(next_tgt - now);
    }

#if ASYNC_DELAY_FEATURE_SLEEP
#if defined(__AVR__)
#include <avr/sleep.h>
    // Atomically enter AVR IDLE sleep mode until the next timer tick interrupt.
    static void async_delay_sleep_idle(void)
    {
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_enable();
#if defined(__GNUC__) || defined(__clang__)
        sei();
        sleep_cpu();
#else
#asm("sei")
#asm("sleep")
#endif
        sleep_disable();
    }
#else
    // Host simulation fallback for sleep_idle
    static void async_delay_sleep_idle(void)
    {
        (void)0;
    }
#endif
#endif

    // Cancel all active and allocated delays in one call.
    static void async_delay_cancel_all(void)
    {
        unsigned char i;
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_CRIT_DECL
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif
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
        _async_next_target = 0;
#endif
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
        }
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_REST_SREG();
#endif
    }

    // ISR-context: process slot i that has been confirmed expired.
    // clr = ~(1<<i), precomputed by the caller - a compile-time literal when the
    // tick is unrolled, which is what keeps __LSLW12 out of the hot path.
    // Interrupts are already off here (ISR context), so the mask read-modify-writes
    // need no extra guard.
    static void _async_delay_expire_slot(unsigned char i, async_mask_t clr)
    {
#if ASYNC_DELAY_CALLBACK_RESCHEDULE && !ASYNC_DELAY_DEFERRED_CALLBACKS && !ASYNC_DELAY_DISABLE_CALLBACKS
        async_delay_cb_t cb;
#endif

#if ASYNC_DELAY_CALLBACK_RESCHEDULE
#if !ASYNC_DELAY_DISABLE_PERIODIC
        if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
        {
            // Periodic: re-arm FIRST so the slot stays ACTIVE for its next
            // cycle; the callback then runs with this slot still busy, so a
            // self-reschedule from the callback lands in a DIFFERENT slot.
            _AD_TARGET(i) += _AD_DUR(i);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
            _async_pending_mask |= (async_mask_t)~clr;
#else
#if !ASYNC_DELAY_DISABLE_CALLBACKS
            _AD_CB(i)(i);
#endif
#endif
            // state stays ACTIVE
        }
        else
#endif
        {
            // One-shot: free the slot BEFORE the callback so a
            // self-reschedule can reuse this very slot.
#if !ASYNC_DELAY_DISABLE_CALLBACKS
            if (_AD_CB(i) != (void *)0)
            {
#if ASYNC_DELAY_OPT_BITMASK
                _async_active_mask &= clr;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
                _async_used_mask &= clr; // one-shot: fully deallocated
#endif
                _AD_FLAGS(i) = ASYNC_SLOT_FREE;
#if ASYNC_DELAY_DEFERRED_CALLBACKS
                // Do NOT clear _AD_CB here - async_delay_poll() still needs it.
                _async_pending_mask |= (async_mask_t)~clr;
#else
                // Take a copy, null the slot's pointer, then call. A freed slot
                // with a live callback pointer would be an ICALL target if its
                // mask bit were ever wrongly set.
                cb = _AD_CB(i);
                _AD_SET_CB(i, (void *)0);
                cb(i);
#endif
            }
            else
#endif
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
#if !ASYNC_DELAY_DISABLE_PERIODIC
    if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
    {
        // Periodic: fire callback, then re-arm using target +=
        // duration so timing stays steady even if a tick is late.
#if ASYNC_DELAY_DEFERRED_CALLBACKS
        _async_pending_mask |= (async_mask_t)~clr;
#else
#if !ASYNC_DELAY_DISABLE_CALLBACKS
        _AD_CB(i)(i);
#endif
#endif
        _AD_TARGET(i) += _AD_DUR(i);
        // state stays ACTIVE
    }
    else
#endif
    {
        // One-shot: fire callback (if any) then free the slot.
#if !ASYNC_DELAY_DISABLE_CALLBACKS
        if (_AD_CB(i) != (void *)0)
        {
#if ASYNC_DELAY_DEFERRED_CALLBACKS
            _async_pending_mask |= (async_mask_t)~clr;
#else
            _AD_CB(i)(i);
#endif
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= clr;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
            _async_used_mask &= clr;
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
        }
        else
#endif
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
// Per-slot tick body. `n` MUST be a compile-time constant so that (1<<n),
// ~(1<<n) and &_async_slots[n] all fold to literals / absolute addresses.
// That is the entire point: with a runtime index CodeVisionAVR emits a call to
// __LSLW12 (a bit-at-a-time shift loop) for (1<<i), a MUL for the i*sizeof
// struct offset, and a __GETW1P call to load .target - roughly 72 cycles per
// slot instead of ~13.
// Do NOT introduce a runtime-indexed caller; it silently undoes all of it.
#define _AD_TICK_SLOT(n)                                          \
    if (_ad_m & _AD_SLOT_BIT(n))                                  \
    {                                                             \
        if (_ASYNC_REACHED(_ad_now, _AD_TARGET(n)))               \
            _async_delay_expire_slot((unsigned char)(n),          \
                                     _AD_SLOT_CLR(n));            \
    }

// Slots above MAX_SLOTS expand to nothing, so the sweep below is one macro
// regardless of slot count. Both tick shapes (SPLIT_TICK 1 and 0) use it, so
// the A/B switch does not duplicate the body.
#if ASYNC_DELAY_MAX_SLOTS > 1
#define _AD_TICK_S1 _AD_TICK_SLOT(1)
#else
#define _AD_TICK_S1
#endif
#if ASYNC_DELAY_MAX_SLOTS > 2
#define _AD_TICK_S2 _AD_TICK_SLOT(2)
#else
#define _AD_TICK_S2
#endif
#if ASYNC_DELAY_MAX_SLOTS > 3
#define _AD_TICK_S3 _AD_TICK_SLOT(3)
#else
#define _AD_TICK_S3
#endif
#if ASYNC_DELAY_MAX_SLOTS > 4
#define _AD_TICK_S4 _AD_TICK_SLOT(4)
#else
#define _AD_TICK_S4
#endif
#if ASYNC_DELAY_MAX_SLOTS > 5
#define _AD_TICK_S5 _AD_TICK_SLOT(5)
#else
#define _AD_TICK_S5
#endif
#if ASYNC_DELAY_MAX_SLOTS > 6
#define _AD_TICK_S6 _AD_TICK_SLOT(6)
#else
#define _AD_TICK_S6
#endif
#if ASYNC_DELAY_MAX_SLOTS > 7
#define _AD_TICK_S7 _AD_TICK_SLOT(7)
#else
#define _AD_TICK_S7
#endif
#if ASYNC_DELAY_MAX_SLOTS > 8
#define _AD_TICK_S8 _AD_TICK_SLOT(8)
#else
#define _AD_TICK_S8
#endif
#if ASYNC_DELAY_MAX_SLOTS > 9
#define _AD_TICK_S9 _AD_TICK_SLOT(9)
#else
#define _AD_TICK_S9
#endif
#if ASYNC_DELAY_MAX_SLOTS > 10
#define _AD_TICK_S10 _AD_TICK_SLOT(10)
#else
#define _AD_TICK_S10
#endif
#if ASYNC_DELAY_MAX_SLOTS > 11
#define _AD_TICK_S11 _AD_TICK_SLOT(11)
#else
#define _AD_TICK_S11
#endif
#if ASYNC_DELAY_MAX_SLOTS > 12
#define _AD_TICK_S12 _AD_TICK_SLOT(12)
#else
#define _AD_TICK_S12
#endif
#if ASYNC_DELAY_MAX_SLOTS > 13
#define _AD_TICK_S13 _AD_TICK_SLOT(13)
#else
#define _AD_TICK_S13
#endif
#if ASYNC_DELAY_MAX_SLOTS > 14
#define _AD_TICK_S14 _AD_TICK_SLOT(14)
#else
#define _AD_TICK_S14
#endif
#if ASYNC_DELAY_MAX_SLOTS > 15
#define _AD_TICK_S15 _AD_TICK_SLOT(15)
#else
#define _AD_TICK_S15
#endif

#define _AD_TICK_SWEEP()                                       \
    _AD_TICK_SLOT(0)                                           \
    _AD_TICK_S1 _AD_TICK_S2 _AD_TICK_S3 _AD_TICK_S4            \
        _AD_TICK_S5 _AD_TICK_S6 _AD_TICK_S7 _AD_TICK_S8        \
            _AD_TICK_S9 _AD_TICK_S10 _AD_TICK_S11 _AD_TICK_S12 \
                _AD_TICK_S13 _AD_TICK_S14 _AD_TICK_S15

#else /* !ASYNC_DELAY_OPT_UNROLL_TICK */

// Runtime bit scan. Kept so the unroll can be A/B measured; this is the shape
// that pays __LSLW12 + MUL + __GETW1P per slot.
#define _AD_TICK_SWEEP()                                       \
    {                                                          \
        unsigned char i;                                       \
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)            \
        {                                                      \
            if (_ad_m & _AD_SLOT_BIT(i))                       \
            {                                                  \
                if (_ASYNC_REACHED(_ad_now, _AD_TARGET(i)))    \
                    _async_delay_expire_slot(i,                \
                                             _AD_SLOT_CLR(i)); \
            }                                                  \
        }                                                      \
    }
#endif

// The recompute after a sweep is UNCONDITIONAL, not gated on "did something
// fire". Reaching a sweep means _ASYNC_REACHED(now, _async_next_target) held,
// and _async_next_target is always the exact minimum of the ACTIVE targets
// (start() takes the min, cancel() recomputes, every sweep recomputes). So some
// ACTIVE slot has target == _async_next_target and satisfies the same compare
// -> at least one slot fired -> the cached minimum is always stale here.
// This is a speed argument only; an unconditional recompute is correct either
// way (it is O(N) and idempotent). Never make it conditional on anything
// weaker than the above.
#if ASYNC_DELAY_OPT_NEXT_TARGET
#define _AD_TICK_AFTER_SWEEP() _async_recompute_next()
#else
#define _AD_TICK_AFTER_SWEEP()
#endif

#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_OPT_SPLIT_TICK
    // Walk the ACTIVE slots and expire the ones that are due.
    // Split out of async_delay_tick() deliberately: this is where the register
    // locals live, so CodeVisionAVR's __SAVELOCR spill is paid ONLY on the ticks
    // that have real work - not on every idle tick.
    // ISR context; interrupts are already off.
    static void _async_delay_tick_walk(void)
    {
        async_tick_t _ad_now;
        async_mask_t _ad_m;

        // Read each volatile once. Without this the per-slot compares below would
        // reload the counter with LDS/LDS every time (4 cycles x slot).
        _ad_now = _async_tick_counter;
        _ad_m = _async_active_mask;

        _AD_TICK_SWEEP();
        _AD_TICK_AFTER_SWEEP();
    }
#endif

    // MUST be called from timer ISR at exactly ASYNC_DELAY_TICK_HZ rate.
    // Increments the tick counter, then gets out as fast as possible unless a slot
    // is actually due.
    // WARNING: Callbacks execute in ISR context - keep them very short!
    //          (unless ASYNC_DELAY_DEFERRED_CALLBACKS=1, see async_delay_poll)
    static void async_delay_tick(void)
    {
#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_OPT_SPLIT_TICK
        // NO locals in this function, deliberately. CodeVisionAVR spills register
        // locals with RCALL __SAVELOCR4 on entry and __LOADLOCR4 on exit - ~30
        // cycles charged on EVERY tick, including the idle ones that return three
        // instructions later. All the locals live in _async_delay_tick_walk().

        // Counter FIRST, always. Every early return below must not skip it or
        // delays drift whenever no slot happens to be active.
        _async_tick_counter++;

        if (_async_active_mask == 0)
            return; // idle tick

#if ASYNC_DELAY_OPT_NEXT_TARGET
        // Nothing due yet: one wrap-safe compare and out, regardless of how many
        // slots are active. This is what makes the tick O(1).
        if (!_ASYNC_REACHED(_async_tick_counter, _async_next_target))
            return;
#endif

        _async_delay_tick_walk();
#elif ASYNC_DELAY_OPT_BITMASK
    // SPLIT_TICK=0: single-function shape with the locals hoisted here, for
    // A/B measurement against the split above. Same behavior, but the
    // __SAVELOCR spill is charged to idle ticks too.
    async_tick_t _ad_now;
    async_mask_t _ad_m;

    _ad_now = (async_tick_t)(_async_tick_counter + 1);
    _async_tick_counter = _ad_now;

    _ad_m = _async_active_mask;
    if (_ad_m == 0)
        return;

#if ASYNC_DELAY_OPT_NEXT_TARGET
    if (!_ASYNC_REACHED(_ad_now, _async_next_target))
        return;
#endif

    _AD_TICK_SWEEP();
    _AD_TICK_AFTER_SWEEP();
#else
    // Legacy: full linear scan (behavior identical to before when all flags 0).
    unsigned char i;
    async_tick_t _ad_now;
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
                _async_delay_expire_slot(i, _AD_SLOT_CLR(i));
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
        async_mask_t p, slotbit;
        unsigned char i;
#if ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_CRIT_DECL
#endif

        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
        p = _async_pending_mask;
        _async_pending_mask = 0;
        _ASYNC_REST_SREG();
#if !ASYNC_DELAY_FIX_ATOMIC_MASK
#if defined(__GNUC__) || defined(__clang__)
        sei();
#else
#asm("sei") // no SREG copy to restore: fall back to enabling
#endif
#endif

        if (p == 0)
            return;

        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            slotbit = _AD_SLOT_BIT(i);
            if (p & slotbit)
            {
#if !ASYNC_DELAY_DISABLE_CALLBACKS
                if (_AD_CB(i) != (void *)0)
                    _AD_CB(i)(i);
#endif
                p &= (async_mask_t)~slotbit;
                if (p == 0)
                    break; // early exit: all pending callbacks processed
            }
        }
    }
#endif

    // ========================================================================
    // AVR HARDWARE TIMER AUTO-CONFIGURATION MODULE (ATmega8 / 16 / 32)
    // ========================================================================
    // One-line macros and helper functions to initialize hardware timers in
    // CTC (Clear Timer on Compare Match) mode for rock-solid zero-jitter ticks.
    // Supports 1, 2, 4, 8, 16 MHz clock frequencies.
    // ========================================================================

#if defined(__AVR__) || defined(_MEGA8_) || defined(_MEGA16_) || defined(_MEGA32_) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__) || defined(ASYNC_DELAY_TEST_HARDWARE_MACROS)

    // Timer 1 (16-bit CTC Mode - Recommended for best precision & zero jitter)
    // In ISR, call async_delay_tick().
    //   CodeVisionAVR : interrupt [TIM1_COMPA] void timer1_compa_isr(void) { async_delay_tick(); }
    //   AVR-GCC       : ISR(TIMER1_COMPA_vect) { async_delay_tick(); }
#define ASYNC_DELAY_SETUP_TIMER1_CTC_16MHZ_1KHZ() do { \
        TCCR1A = 0x00; \
        TCCR1B = 0x0A; /* CTC mode (WGM12=1), Prescaler /8 (CS11=1) */ \
        TCNT1H = 0x00; TCNT1L = 0x00; \
        OCR1AH = 0x07; OCR1AL = 0xCF; /* 16MHz / (8 * 1000Hz) - 1 = 1999 (0x07CF) */ \
        TIMSK |= 0x10; /* Enable OCIE1A */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER1_CTC_8MHZ_1KHZ() do { \
        TCCR1A = 0x00; \
        TCCR1B = 0x0A; /* CTC mode (WGM12=1), Prescaler /8 (CS11=1) */ \
        TCNT1H = 0x00; TCNT1L = 0x00; \
        OCR1AH = 0x03; OCR1AL = 0xE7; /* 8MHz / (8 * 1000Hz) - 1 = 999 (0x03E7) */ \
        TIMSK |= 0x10; /* Enable OCIE1A */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER1_CTC_4MHZ_1KHZ() do { \
        TCCR1A = 0x00; \
        TCCR1B = 0x0A; /* CTC mode (WGM12=1), Prescaler /8 (CS11=1) */ \
        TCNT1H = 0x00; TCNT1L = 0x00; \
        OCR1AH = 0x01; OCR1AL = 0xF3; /* 4MHz / (8 * 1000Hz) - 1 = 499 (0x01F3) */ \
        TIMSK |= 0x10; /* Enable OCIE1A */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER1_CTC_2MHZ_1KHZ() do { \
        TCCR1A = 0x00; \
        TCCR1B = 0x0A; /* CTC mode (WGM12=1), Prescaler /8 (CS11=1) */ \
        TCNT1H = 0x00; TCNT1L = 0x00; \
        OCR1AH = 0x00; OCR1AL = 0xF9; /* 2MHz / (8 * 1000Hz) - 1 = 249 (0x00F9) */ \
        TIMSK |= 0x10; /* Enable OCIE1A */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER1_CTC_1MHZ_1KHZ() do { \
        TCCR1A = 0x00; \
        TCCR1B = 0x0A; /* CTC mode (WGM12=1), Prescaler /8 (CS11=1) */ \
        TCNT1H = 0x00; TCNT1L = 0x00; \
        OCR1AH = 0x00; OCR1AL = 0x7C; /* 1MHz / (8 * 1000Hz) - 1 = 124 (0x007C) */ \
        TIMSK |= 0x10; /* Enable OCIE1A */ \
    } while (0)

    // Timer 2 (8-bit CTC Mode)
    //   CodeVisionAVR : interrupt [TIM2_COMP] void timer2_comp_isr(void) { async_delay_tick(); }
    //   AVR-GCC       : ISR(TIMER2_COMP_vect) { async_delay_tick(); }
#define ASYNC_DELAY_SETUP_TIMER2_CTC_16MHZ_1KHZ() do { \
        TCCR2 = 0x0C; /* CTC mode (WGM21=1), Prescaler /64 (CS22=1) */ \
        TCNT2 = 0x00; \
        OCR2  = 249;  /* 16MHz / (64 * 1000Hz) - 1 = 249 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER2_CTC_8MHZ_1KHZ() do { \
        TCCR2 = 0x0C; /* CTC mode (WGM21=1), Prescaler /64 (CS22=1) */ \
        TCNT2 = 0x00; \
        OCR2  = 124;  /* 8MHz / (64 * 1000Hz) - 1 = 124 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER2_CTC_4MHZ_1KHZ() do { \
        TCCR2 = 0x0B; /* CTC mode (WGM21=1), Prescaler /32 (CS21=1, CS20=1) */ \
        TCNT2 = 0x00; \
        OCR2  = 124;  /* 4MHz / (32 * 1000Hz) - 1 = 124 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER2_CTC_2MHZ_1KHZ() do { \
        TCCR2 = 0x0A; /* CTC mode (WGM21=1), Prescaler /8 (CS21=1) */ \
        TCNT2 = 0x00; \
        OCR2  = 249;  /* 2MHz / (8 * 1000Hz) - 1 = 249 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER2_CTC_1MHZ_1KHZ() do { \
        TCCR2 = 0x0A; /* CTC mode (WGM21=1), Prescaler /8 (CS21=1) */ \
        TCNT2 = 0x00; \
        OCR2  = 124;  /* 1MHz / (8 * 1000Hz) - 1 = 124 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

    // Dynamic Timer 1 initialization function for custom frequencies
    static void async_delay_hw_timer1_init(unsigned long f_cpu_hz, unsigned int tick_hz)
    {
        unsigned long top;
        TCCR1A = 0x00;
        if (tick_hz == 0)
            tick_hz = 1000;
        top = (f_cpu_hz / (8UL * (unsigned long)tick_hz)) - 1UL;
        TCNT1H = 0x00;
        TCNT1L = 0x00;
        OCR1AH = (unsigned char)((top >> 8) & 0xFF);
        OCR1AL = (unsigned char)(top & 0xFF);
        TCCR1B = 0x0A; /* CTC mode, /8 prescaler */
        TIMSK |= 0x10; /* Enable OCIE1A */
    }

    // Dynamic Timer 2 initialization function for custom frequencies (8-bit CTC mode)
    static void async_delay_hw_timer2_init(unsigned long f_cpu_hz, unsigned int tick_hz)
    {
        unsigned long top;
        unsigned char prescaler_bits;
        unsigned char ocr_val;

        if (tick_hz == 0)
            tick_hz = 1000;

        // Select optimal prescaler to keep OCR2 within 8-bit range (0..255)
        top = (f_cpu_hz / (8UL * (unsigned long)tick_hz)) - 1UL;
        if (top <= 255UL) {
            prescaler_bits = 0x0A; // /8 prescaler (WGM21=1, CS21=1)
            ocr_val = (unsigned char)top;
        } else {
            top = (f_cpu_hz / (32UL * (unsigned long)tick_hz)) - 1UL;
            if (top <= 255UL) {
                prescaler_bits = 0x0B; // /32 prescaler
                ocr_val = (unsigned char)top;
            } else {
                top = (f_cpu_hz / (64UL * (unsigned long)tick_hz)) - 1UL;
                if (top <= 255UL) {
                    prescaler_bits = 0x0C; // /64 prescaler
                    ocr_val = (unsigned char)top;
                } else {
                    top = (f_cpu_hz / (128UL * (unsigned long)tick_hz)) - 1UL;
                    if (top <= 255UL) {
                        prescaler_bits = 0x0D; // /128 prescaler
                        ocr_val = (unsigned char)top;
                    } else {
                        top = (f_cpu_hz / (256UL * (unsigned long)tick_hz)) - 1UL;
                        if (top <= 255UL) {
                            prescaler_bits = 0x0E; // /256 prescaler
                            ocr_val = (unsigned char)top;
                        } else {
                            top = (f_cpu_hz / (1024UL * (unsigned long)tick_hz)) - 1UL;
                            prescaler_bits = 0x0F; // /1024 prescaler
                            if (top > 255UL) {
                                ocr_val = 255;
                            } else {
                                ocr_val = (unsigned char)top;
                            }
                        }
                    }
                }
            }
        }

        TCNT2 = 0x00;
        OCR2  = ocr_val;
        TCCR2 = prescaler_bits; /* CTC mode + prescaler */
        TIMSK |= 0x80;         /* Enable OCIE2 */
    }

#endif /* AVR target */

#ifdef __cplusplus
}
#endif

#endif /* _ASYNC_DELAY_INCLUDED_ */
