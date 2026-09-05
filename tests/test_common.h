// test_common.h - shared bookkeeping for the host-side tests (plan 006).
// Included FIRST by every test_*.c file. NOT part of the firmware.
//
// Style deliberately matches the firmware (C89: all locals at block top,
// 4-space indent, // comments, (void *)0 for NULL) so this file doubles as a
// C89 canary; gcc compiles every driver TU with -std=c89 -pedantic.
#ifndef _ASYNC_DELAY_TEST_COMMON_
#define _ASYNC_DELAY_TEST_COMMON_

#include <stdio.h>
#include <string.h>
#include "host_stub.h"
#include "build/async_delay_host.h"

// Durations must stay under half the counter range (ARCHITECTURE.md 6.1), so
// the 8-bit combo uses smaller values than 16/32-bit.
#if ASYNC_DELAY_TIMER_BITS == 8
#define T_DUR 40       // a one-shot that fires well before any wrap
#define T_DUR2 20
#define T_LONG 120     // a delay that outlives every earlier test in the 8-bit run
#define T_WRAP_BASE 200   // start a T_WRAP_DUR delay near counter 200 -> wraps at 256
#define T_WRAP_DUR 80
#else
#define T_DUR 1000
#define T_DUR2 500
#define T_LONG 5000
#define T_WRAP_BASE 65000  // 16-bit wrap near 65536; 32-bit fires well before any wrap
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

static void t_fail(const char *what, long expected, long actual)
{
    // g_fail counts so main can exit non-zero; g_case names the failing test.
    printf("  FAIL %s: %s expected=%ld got=%ld\n", g_case, what, expected, actual);
    g_fail++;
}

// T_EQ(what, EXPECTED, actual) - call sites pass the expected value FIRST.
#define T_EQ(what, expected, actual)                                          \
    do {                                                                      \
        if ((long)(expected) != (long)(actual))                               \
            t_fail((what), (long)(expected), (long)(actual));                 \
    } while (0)

// Self-reschedule bookkeeping, written by test_resched.c's callbacks and
// cleared by t_reset() - declared BEFORE t_reset so the memset calls compile.
// g_self_slot records where a re-start from inside a callback landed, keyed by
// the firing slot; g_per_self_slot records the one-shot started by T10's
// periodic callback. ASYNC_DELAY_NO_SLOT marks "never started".
static unsigned char g_self_slot[8];
static unsigned long g_self_start_at[8];
static unsigned char g_per_self_slot = ASYNC_DELAY_NO_SLOT;

static void t_reset(void)
{
    // NOTE: g_fail is cumulative on purpose - never reset here, so one binary
    // exit code reports the whole suite.
    g_ev_n = 0;
    g_now  = 0;
    memset(g_ev_slot, 0, sizeof(g_ev_slot));
    memset(g_ev_tick, 0, sizeof(g_ev_tick));
    memset(g_self_slot, 0, sizeof(g_self_slot));
    memset(g_self_start_at, 0, sizeof(g_self_start_at));
    g_per_self_slot = ASYNC_DELAY_NO_SLOT;
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

#if ASYNC_DELAY_DEFERRED_CALLBACKS
static void maybe_poll(void)
{
    async_delay_poll();
}
#else
static void maybe_poll(void)
{
}
#endif

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

// Generic callbacks used by the tests below.
static void cb_a(unsigned char slot_id)
{
    t_record(slot_id);
}

static void cb_b(unsigned char slot_id)
{
    t_record(slot_id);
}

#endif
