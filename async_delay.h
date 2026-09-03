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
#pragma used+

typedef struct {
    async_tick_t     target;    // tick when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    state;     // FREE / ACTIVE / EXPIRED
    unsigned char    repeat;    // 1 = periodic (auto re-arm), 0 = one-shot
} _async_slot_t;

static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
static volatile async_tick_t _async_tick_counter;

// ---------- Public API Implementation ----------

// Initialize all slots to FREE and reset tick counter.
// MUST be called once at startup before any other async_delay function.
static void async_delay_init(void)
{
    unsigned char i;
    _async_tick_counter = 0;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        _async_slots[i].state = ASYNC_SLOT_FREE;
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
        if (_async_slots[i].state == ASYNC_SLOT_FREE)
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
            _async_slots[i].duration = duration;
            _async_slots[i].target   = now + duration;
            _async_slots[i].callback = callback;
            _async_slots[i].repeat   = repeat;
            _async_slots[i].state    = ASYNC_SLOT_ACTIVE;
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

    if (_async_slots[slot_id].state == ASYNC_SLOT_EXPIRED)
    {
        _async_slots[slot_id].state = ASYNC_SLOT_FREE;
        return 1;
    }
    return 0;
}

// Cancel an active or expired delay. Frees the slot.
// Safe to call with invalid slot_id - does nothing silently.
static void async_delay_cancel(unsigned char slot_id)
{
    if (slot_id < ASYNC_DELAY_MAX_SLOTS)
        _async_slots[slot_id].state = ASYNC_SLOT_FREE;
}

// MUST be called from timer ISR at exactly ASYNC_DELAY_TICK_HZ rate.
// Increments tick counter, checks all active slots, fires callbacks.
// WARNING: Callbacks execute in ISR context - keep them very short!
static void async_delay_tick(void)
{
    unsigned char i;
    async_tick_t half;

    _async_tick_counter++;

    // Half of the tick range - used for the wrap-safe comparison.
    half = (async_tick_t)(~((async_tick_t)0) >> 1);

    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (_async_slots[i].state == ASYNC_SLOT_ACTIVE)
        {
            // Wrap-safe "now is past target" test:
            // if (tick - target) < half_range, the delay has elapsed.
            // This stays correct across a 16/32-bit counter wrap, as long
            // as the delay is shorter than half the tick range.
            if ((async_tick_t)(_async_tick_counter - _async_slots[i].target) < half)
            {
                if (_async_slots[i].repeat && _async_slots[i].callback != (void *)0)
                {
                    // Periodic: fire callback, then re-arm using target +=
                    // duration so timing stays steady even if a tick is late.
                    _async_slots[i].callback(i);
                    _async_slots[i].target += _async_slots[i].duration;
                    // state stays ACTIVE
                }
                else
                {
                    // One-shot: fire callback (if any) then free the slot.
                    if (_async_slots[i].callback != (void *)0)
                    {
                        _async_slots[i].callback(i);
                        _async_slots[i].state = ASYNC_SLOT_FREE;
                    }
                    else
                    {
                        // Polling mode: mark EXPIRED, user checks with elapsed()
                        _async_slots[i].state = ASYNC_SLOT_EXPIRED;
                    }
                }
            }
        }
    }
}

#pragma used-

#endif
