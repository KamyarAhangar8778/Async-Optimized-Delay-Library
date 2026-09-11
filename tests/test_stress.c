// test_stress.c - Host-based stress & concurrency test suite for async_delay.h
// Tests high-load interleaving, continuous wrap-around, self-rescheduling, and sleep math.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ASYNC_DELAY_TICK_HZ
#define ASYNC_DELAY_TICK_HZ 1000
#endif

#include "../async_delay.h"

static int stress_total = 0;
static int stress_passed = 0;

#define STRESS_ASSERT(cond, msg) do { \
    stress_total++; \
    if (cond) { \
        stress_passed++; \
    } else { \
        printf("  [FAIL] Line %d: %s\n", __LINE__, msg); \
    } \
} while (0)

#if !ASYNC_DELAY_DISABLE_CALLBACKS
static volatile unsigned int self_resched_count = 0;
static volatile unsigned char resched_new_slot = 0xFF;

static void cb_self_reschedule(unsigned char slot_id)
{
    self_resched_count++;
    (void)slot_id;
    if (self_resched_count < 5)
    {
        // Re-arm delay for another 10 ticks from within callback
        resched_new_slot = async_delay_start(10, cb_self_reschedule);
    }
}

static void test_self_reschedule(void)
{
    unsigned int k;
    printf("Running test_self_reschedule...\n");
    async_delay_init();
    self_resched_count = 0;
    resched_new_slot = 0xFF;

    async_delay_start(10, cb_self_reschedule);

    for (k = 0; k < 60; k++)
    {
        async_delay_tick();
#if ASYNC_DELAY_DEFERRED_CALLBACKS
        async_delay_poll();
#endif
    }

    STRESS_ASSERT(self_resched_count == 5, "self-rescheduling fired exactly 5 times");
    STRESS_ASSERT(async_delay_active_count() == 0, "all self-rescheduled slots freed cleanly");
}
#endif

// -------------------------------------------------------------
// Test: Dynamic Power-Saving Earliest Target Tracking
// -------------------------------------------------------------
static void test_dynamic_earliest_tracking(void)
{
    unsigned char id_long, id_med, id_short;
    printf("Running test_dynamic_earliest_tracking...\n");
    async_delay_init();

    id_long = async_delay_start(100, (void *)0);
    STRESS_ASSERT(async_delay_ticks_until_next() == 100, "next target is 100");

    // Add shorter delay
    id_med = async_delay_start(50, (void *)0);
    STRESS_ASSERT(async_delay_ticks_until_next() == 50, "next target updated to 50");

    // Add even shorter delay
    id_short = async_delay_start(20, (void *)0);
    STRESS_ASSERT(async_delay_ticks_until_next() == 20, "next target updated to 20");

    // Cancel shortest delay -> next target should dynamically revert to 50
    async_delay_cancel(id_short);
    STRESS_ASSERT(async_delay_ticks_until_next() == 50, "recomputed next target is 50 after cancel");

    // Cancel medium delay -> next target should dynamically revert to 100
    async_delay_cancel(id_med);
    STRESS_ASSERT(async_delay_ticks_until_next() == 100, "recomputed next target is 100 after cancel");

    async_delay_cancel(id_long);
    STRESS_ASSERT(async_delay_ticks_until_next() == 0, "next target is 0 when idle");
}

// -------------------------------------------------------------
// Test: Multi-Wrap Continuous Simulation
// -------------------------------------------------------------
static void test_continuous_wrap_simulation(void)
{
    unsigned int t;
    unsigned char id1, id2;
    unsigned int fired1 = 0, fired2 = 0;
    printf("Running test_continuous_wrap_simulation...\n");
    async_delay_init();

    // Start with 2 rolling polling timers (period 7 and 13)
    id1 = async_delay_start(7, (void *)0);
    id2 = async_delay_start(13, (void *)0);

    for (t = 1; t <= 3000; t++)
    {
        async_delay_tick();

        if (async_delay_elapsed(id1))
        {
            fired1++;
            id1 = async_delay_start(7, (void *)0);
        }
        if (async_delay_elapsed(id2))
        {
            fired2++;
            id2 = async_delay_start(13, (void *)0);
        }
    }

    STRESS_ASSERT(fired1 == 3000 / 7, "timer1 fired exact expected count across wrap");
    STRESS_ASSERT(fired2 == 3000 / 13, "timer2 fired exact expected count across wrap");

    async_delay_cancel_all();
    STRESS_ASSERT(async_delay_active_count() == 0, "clean shutdown after continuous wrap");
}

// -------------------------------------------------------------
// Test: Edge Cases (0-tick, 1-tick)
// -------------------------------------------------------------
static void test_boundary_delays(void)
{
    unsigned char id0, id1;
    printf("Running test_boundary_delays...\n");
    async_delay_init();

    // 0-tick delay should expire on very first tick
    id0 = async_delay_start(0, (void *)0);
    STRESS_ASSERT(id0 != ASYNC_DELAY_NO_SLOT, "0-tick delay allocated");

    // 1-tick delay
    id1 = async_delay_start(1, (void *)0);
    STRESS_ASSERT(id1 != ASYNC_DELAY_NO_SLOT, "1-tick delay allocated");

    async_delay_tick();
    STRESS_ASSERT(async_delay_elapsed(id0) == 1, "0-tick delay expired on tick 1");
    STRESS_ASSERT(async_delay_elapsed(id1) == 1, "1-tick delay expired on tick 1");
}

int main(void)
{
    printf("========================================\n");
    printf("  async_delay Stress & Edge-Case Suite\n");
    printf("  Config: BITS=%d, SLOTS=%d\n", ASYNC_DELAY_TIMER_BITS, ASYNC_DELAY_MAX_SLOTS);
    printf("========================================\n");

#if !ASYNC_DELAY_DISABLE_CALLBACKS
    test_self_reschedule();
#endif
    test_dynamic_earliest_tracking();
    test_continuous_wrap_simulation();
    test_boundary_delays();

    printf("\n----------------------------------------\n");
    printf("Stress Tests Run: %d | Passed: %d | Failed: %d\n",
           stress_total, stress_passed, stress_total - stress_passed);
    printf("----------------------------------------\n");

    return (stress_passed == stress_total) ? 0 : 1;
}
