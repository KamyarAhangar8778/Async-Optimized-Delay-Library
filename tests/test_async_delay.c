// test_async_delay.c - host-side behavioral tests for async_delay.h (plan 006).
// Compiled once per flag combo by tests/make_host.py. NOT part of the firmware.
//
// Style deliberately matches the firmware (C89: all locals at block top, 4-space
// indent, // comments, (void *)0 for NULL) so this file doubles as a C89 canary;
// gcc runs it with -std=gnu89, which enforces the same declaration-first rule
// CodeVisionAVR needs.
//
// IMPORTANT DESIGN CONSTRAINT (plans/006 plan 4.4): the library's
// _async_tick_counter is file-static, so the driver cannot write it directly to
// force a wrap. Instead every "tick" here is:  g_now++ ; async_delay_tick().
// g_now is the driver's mirror of the counter and is what callbacks observe via
// t_record. The firmware is single-threaded host code here, so reading >8-bit
// quantities needs no critical section.

#include <stdio.h>
#include <string.h>
#include "host_stub.h"
#include "build/async_delay_host.h"
#if ASYNC_DELAY_DEFERRED_CALLBACKS
#define _HOST_DEFERRED 1
#else
#define _HOST_DEFERRED 0
#endif

// Durations must stay under half the counter range (ARCHITECTURE.md 6.1), so the
// 8-bit combo uses smaller values than 16/32-bit.
#if ASYNC_DELAY_TIMER_BITS == 8
#define T_DUR 40       // a one-shot that fires well before any wrap
#define T_DUR2 20
#define T_WRAP_BASE 250   // start a 40-tick delay at counter 250 -> wraps at 256
#define T_WRAP_DUR 40
#else
#define T_DUR 1000
#define T_DUR2 500
#define T_WRAP_BASE 65000  // 16-bit wrap near 65536; 32-bit uses larger
#define T_WRAP_DUR 1000
#endif

#define T_MAX_EV 128

// ---------- bookkeeping ----------
static unsigned char g_ev_slot[T_MAX_EV];
static unsigned long g_ev_tick[T_MAX_EV];   // driver's g_now at fire time
static unsigned int  g_ev_n;
static unsigned long g_now;
static int           g_fail;
static const char   *g_case = "?";
static unsigned char g_start_result;        // returned slot id from start()

static void t_fail(const char *what, long got, long want)
{
    printf("  FAIL %s: %s got=%ld want=%ld\n", g_case, what, got, want);
    g_fail++;
}

#define T_EQ(what, got, want)                                                 \
    do {                                                                      \
        if ((long)(got) != (long)(want))                                      \
            t_fail((what), (long)(got), (long)(want));                        \
    } while (0)

static void t_reset(void)
{
    g_ev_n = 0;
    g_now  = 0;
    g_fail = (g_fail) ? g_fail : 0;   // keep cumulative, do not reset failures
}

// Run n ticks; each is a counter increment followed by the library tick.
static void ticks(unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i++)
    {
        g_now++;
        async_delay_tick();
    }
}

// Fire any deferred callbacks (only meaningful under _HOST_DEFERRED).
static void maybe_poll(void)
{
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
}

// Record a callback event (called from the library's callback funcs).
static void t_record(unsigned char slot_id)
{
    if (g_ev_n < T_MAX_EV)
    {
        g_ev_slot[g_ev_n] = slot_id;
        g_ev_tick[g_ev_n] = g_now;
    }
    g_ev_n++;
}

// Generic callbacks. cb_count increments the event history per slot, but the
// tests mostly inspect g_ev_tick/g_ev_slot; a callback that self-reschedules
// is cb_self01 / cb_self02 below.

static void cb_a(unsigned char slot_id)
{
    t_record(slot_id);
}

static void cb_b(unsigned char slot_id)
{
    t_record(slot_id);
}

// A callback that re-starts a one-shot on the SAME slot id pattern (copy the
// returned id into a global so the caller can check it).
static unsigned long g_self_start_at[8];
static unsigned char g_self_slot[8];

static void cb_self01(unsigned char slot_id)
{
    unsigned char r;
    t_record(slot_id);
    r = async_delay_start(T_DUR, cb_a);
    g_self_slot[slot_id] = r;          // where did the new delay land?
    g_self_start_at[slot_id] = g_now;  // when
}

// ---------- library accessor shims (read-only, safe) ----------
// The masks are static but volatile; on the host they are plain bytes. We do
// NOT poke them; we only observe public behavior through start/elapsed/cancel
// and the callbacks. That is the whole point: test the public contract.

// ---------- T1: idle counting ----------
static void t_idle_count(void)
{
    g_case = "T1 idle";
    async_delay_init();
    t_reset();
    ticks(1000);
    T_EQ("counter advanced by 1000 ticks", 1000, g_now);
}

// ---------- T2: one-shot callback fires exactly once ----------
static void t_one_shot(void)
{
    g_case = "T2 one-shot";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_DUR, cb_a);
    T_EQ("start returns slot 0", 0, g_start_result);
    ticks(T_DUR - 1);
    T_EQ("no fire before due", 0, g_ev_n);
    maybe_poll();
    T_EQ("still no fire before due (even after poll)", 0, g_ev_n);
    ticks(1);                       // exactly at due
    maybe_poll();
    T_EQ("fired exactly once at due", 1, g_ev_n);
    T_EQ("fired on slot 0", 0, g_ev_slot[0]);
    T_EQ("fired at tick T_DUR", T_DUR, g_ev_tick[0]);
    ticks(T_DUR);                   // past it
    maybe_poll();
    T_EQ("no second fire", 1, g_ev_n);
}

// ---------- T3: polling lifecycle ----------
static void t_polling(void)
{
    g_case = "T3 polling";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("start returns slot 0", 0, g_start_result);
    ticks(T_DUR - 1);
    T_EQ("elapsed 0 before due", 0, async_delay_elapsed(g_start_result));
    ticks(1);
    T_EQ("elapsed 1 at due", 1, async_delay_elapsed(g_start_result));
    T_EQ("elapsed 0 after freed", 0, async_delay_elapsed(g_start_result));
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot reusable after elapsed", 0, g_start_result);
}

// ---------- T4: overflow returns NO_SLOT ----------
static void t_overflow(void)
{
    g_case = "T4 overflow";
    unsigned char i, r;
    async_delay_init();
    t_reset();
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        r = async_delay_start(T_DUR, cb_a);
        T_EQ("slot fills", i, r);
    }
    r = async_delay_start(T_DUR, cb_a);
    T_EQ("overflow returns NO_SLOT", ASYNC_DELAY_NO_SLOT, r);
}

// ---------- T5: cancel prevents firing ----------
static void t_cancel(void)
{
    g_case = "T5 cancel";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_LONG, cb_a);
    async_delay_cancel(g_start_result);
    ticks(T_LONG + 200);
    maybe_poll();
    T_EQ("no fire after cancel", 0, g_ev_n);
    T_EQ("slot is free after cancel", 0, async_delay_start(T_DUR, cb_a));
}

// ---------- T6: periodic phase-lock (fires at exact multiples) ----------
static void t_periodic(void)
{
    g_case = "T6 periodic";
    unsigned long k;
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start_periodic(T_DUR2, cb_a);
    T_EQ("periodic start returns slot 0", 0, g_start_result);
    ticks(T_DUR2 * 10);
    maybe_poll();
    T_EQ("fired 10 times", 10, g_ev_n);
    for (k = 0; k < 10; k++)
        T_EQ("fire tick = exact multiple", k * T_DUR2, g_ev_tick[k]);
}

// ---------- T7: used-vs-active (BUG-1): EXPIRED slot not reused until elapsed ----------
static void t_used_vs_active(void)
{
    g_case = "T7 used-vs-active";
    unsigned char id1;
    async_delay_init();
    t_reset();
    // Two polling slots; let #0 expire (EXPIRED, still allocated).
    id1 = async_delay_start(T_DUR, (void *)0);
    async_delay_start(T_DUR, (void *)0);
    ticks(T_DUR);
    // slot 0 is EXPIRED + allocated; slot 1 is still ACTIVE.
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("EXPIRED slot NOT reused before elapsed", 1, g_start_result); // must pick slot 1, not 0
    // now release it and it becomes reusable
    T_EQ("elapsed on id1 frees it", 1, async_delay_elapsed(id1));
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot 0 reusable after elapsed", 0, g_start_result);
}

// ---------- T8: inverted test (only when FIX_USED_MASK=0) ----------
// The bug this guards: without FIX_USED_MASK, start() hands out an EXPIRED-but-
// unpolled slot, silently destroying its owner's timer. Compiling this assert
// ONLY in the FIX_USED_MASK=0 combo proves the harness can actually SEE the bug.
#if !ASYNC_DELAY_FIX_USED_MASK
static void t_invert_used_bug(void)
{
    g_case = "T8 inverted used-bug";
    unsigned char id1;
    async_delay_init();
    t_reset();
    id1 = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot0 allocated", 0, id1);
    ticks(T_DUR);                         // slot0 expires
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("BUG reproduced: start steals EXPIRED slot 0 (FIX_USED_MASK=0)", 0, g_start_result);
    // Sanity: this is a BUG by design under this flag - the point is the test
    // passes, proving the harness detects what FIX_USED_MASK prevents in T7.
}
#endif

// ---------- T9: self-reschedule, one-shot ----------
static void t_self_oneshot(void)
{
    g_case = "T9 self one-shot";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_DUR, cb_self01);
    T_EQ("start returns slot 0", 0, g_start_result);
    ticks(T_DUR);
    maybe_poll();
    T_EQ("first fire recorded", 1, g_ev_n);
    // cb_self01 ran async_delay_start(T_DUR, cb_a) from inside the callback.
    // RESCHEDULE=1 frees the one-shot slot BEFORE the callback, so the re-start
    // should land on the SAME slot 0 (g_self_slot[0]==0) and fire T_DUR later.
    T_EQ("self-reschedule reused slot 0", 0, g_self_slot[0]);
    ticks(T_DUR);
    maybe_poll();
    T_EQ("self-rescheduled delay fired", 2, g_ev_n);
    T_EQ("re-fired on slot 0", 0, g_ev_slot[1]);
    T_EQ("re-fired T_DUR after first", (long)(g_ev_tick[0] + T_DUR), (long)g_ev_tick[1]);
}
