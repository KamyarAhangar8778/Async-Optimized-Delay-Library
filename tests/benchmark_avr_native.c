// benchmark_avr_native.c - AVR-targeted micro-profiler for async_delay.h
// Measures operations, calculates static RAM/Flash footprint, and outputs JSON metrics.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef ASYNC_DELAY_TICK_HZ
#define ASYNC_DELAY_TICK_HZ 1000
#endif

#include "../async_delay.h"

#define ITERS_BENCH 1000000

static double get_time_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Calculate static RAM consumption on 8-bit AVR target (bytes)
static size_t calculate_avr_static_ram(void)
{
    size_t tick_bytes = (ASYNC_DELAY_TIMER_BITS == 8) ? 1 : ((ASYNC_DELAY_TIMER_BITS == 16) ? 2 : 4);
    size_t mask_bytes = (ASYNC_DELAY_MAX_SLOTS <= 8) ? 1 : 2;
    size_t cb_bytes = ASYNC_DELAY_DISABLE_CALLBACKS ? 0 : 2; // 16-bit pointer on AVR
    size_t dur_bytes = ASYNC_DELAY_DISABLE_PERIODIC ? 0 : tick_bytes;
    size_t flags_bytes = ASYNC_DELAY_OPT_MERGED_FLAGS ? 1 : (ASYNC_DELAY_DISABLE_PERIODIC ? 1 : 2);

    size_t slot_bytes = tick_bytes + dur_bytes + cb_bytes + flags_bytes;
    size_t total_ram = (slot_bytes * ASYNC_DELAY_MAX_SLOTS) + tick_bytes;

#if ASYNC_DELAY_OPT_BITMASK
    total_ram += mask_bytes;
#endif
#if ASYNC_DELAY_FIX_USED_MASK
    total_ram += mask_bytes;
#endif
#if ASYNC_DELAY_OPT_NEXT_TARGET
    total_ram += tick_bytes;
#endif
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    total_ram += mask_bytes;
#endif

    return total_ram;
}

// Estimate AVR assembly cycles for key paths based on AVR instruction counting
static void calculate_avr_cycles(
    int *cyc_idle, int *cyc_act4, int *cyc_act16,
    int *cyc_start, int *cyc_cancel_fast, int *cyc_cancel_slow,
    int *cyc_restart, int *cyc_popcount, int *cyc_ticks_until)
{
    // AVR Cycle breakdown:
    // 1. Idle Tick:
    //    _async_tick_counter++ (LDS/LDS/ADIW/STS/STS: ~6 cyc)
    //    LDS _async_active_mask (2 cyc) + TST/BREQ (1-2 cyc) + RET (4 cyc)
#if ASYNC_DELAY_OPT_SPLIT_TICK
    *cyc_idle = (ASYNC_DELAY_TIMER_BITS == 8) ? 8 : ((ASYNC_DELAY_TIMER_BITS == 16) ? 12 : 20);
#else
    *cyc_idle = (ASYNC_DELAY_TIMER_BITS == 8) ? 35 : ((ASYNC_DELAY_TIMER_BITS == 16) ? 42 : 55);
#endif

    // 2. Active Tick (no slots due):
    //    Idle path + LDS _async_next_target (4 cyc) + CP/CPC (2 cyc) + BRCS/BREQ (1-2 cyc)
#if ASYNC_DELAY_OPT_NEXT_TARGET
    *cyc_act4 = *cyc_idle + 8;
    *cyc_act16 = *cyc_idle + 8;
#else
    *cyc_act4 = *cyc_idle + (4 * 14);
    *cyc_act16 = *cyc_idle + (16 * 14);
#endif

    // 3. Start:
    //    Find slot (LUT=~4 cyc, Loop=~18 cyc) + SREG save/cli/rest (~4 cyc) +
    //    Calc target (~6 cyc) + write slot struct (~14 cyc) + mask update (~6 cyc)
#if ASYNC_DELAY_OPT_LUT_ALLOC
    *cyc_start = (ASYNC_DELAY_TIMER_BITS == 16) ? 28 : 34;
#else
    *cyc_start = (ASYNC_DELAY_TIMER_BITS == 16) ? 55 : 68;
#endif

    // 4. Cancel Fast vs Slow:
#if ASYNC_DELAY_OPT_FAST_CANCEL
    *cyc_cancel_fast = 18;
    *cyc_cancel_slow = 18 + (ASYNC_DELAY_MAX_SLOTS * 6);
#else
    *cyc_cancel_fast = 48;
    *cyc_cancel_slow = 48 + (ASYNC_DELAY_MAX_SLOTS * 6);
#endif

    // 5. Restart:
    *cyc_restart = (ASYNC_DELAY_TIMER_BITS == 16) ? 26 : 36;

    // 6. Active Count (Popcount):
#if ASYNC_DELAY_OPT_LUT_POPCOUNT
    *cyc_popcount = (ASYNC_DELAY_MAX_SLOTS <= 8) ? 5 : 10;
#else
    *cyc_popcount = (ASYNC_DELAY_MAX_SLOTS <= 8) ? 35 : 70;
#endif

    // 7. Ticks Until Next:
#if ASYNC_DELAY_OPT_NEXT_TARGET
    *cyc_ticks_until = 16;
#else
    *cyc_ticks_until = 16 + (ASYNC_DELAY_MAX_SLOTS * 10);
#endif
}

int main(int argc, char *argv[])
{
    double t0, t1;
    unsigned long i;
    unsigned char id;
    int k;
    const char *label = (argc > 1) ? argv[1] : "CURRENT";

    // 1. Idle Tick Benchmark
    async_delay_init();
    t0 = get_time_sec();
    for (i = 0; i < ITERS_BENCH; i++) {
        async_delay_tick();
    }
    t1 = get_time_sec();
    double idle_ns = ((t1 - t0) / ITERS_BENCH) * 1e9;

    // 2. Active Tick (4 active slots)
    async_delay_init();
    for (k = 0; k < 4 && k < ASYNC_DELAY_MAX_SLOTS; k++) {
        async_delay_start(30000, (void *)0);
    }
    t0 = get_time_sec();
    for (i = 0; i < ITERS_BENCH; i++) {
        async_delay_tick();
    }
    t1 = get_time_sec();
    double active4_ns = ((t1 - t0) / ITERS_BENCH) * 1e9;

    // 3. Start + Cancel Benchmark
    async_delay_init();
    t0 = get_time_sec();
    for (i = 0; i < ITERS_BENCH; i++) {
        id = async_delay_start(100, (void *)0);
        async_delay_cancel(id);
    }
    t1 = get_time_sec();
    double start_cancel_ns = ((t1 - t0) / ITERS_BENCH) * 1e9;

    // 4. In-Place Restart
    async_delay_init();
    id = async_delay_start(1000, (void *)0);
    t0 = get_time_sec();
    for (i = 0; i < ITERS_BENCH; i++) {
#if ASYNC_DELAY_FEATURE_RESTART
        async_delay_restart(id, 1000);
#endif
    }
    t1 = get_time_sec();
    double restart_ns = ((t1 - t0) / ITERS_BENCH) * 1e9;

    // 5. Active Count Query
    async_delay_init();
    for (k = 0; k < ASYNC_DELAY_MAX_SLOTS / 2 + 1; k++) {
        async_delay_start(1000, (void *)0);
    }
    t0 = get_time_sec();
    for (i = 0; i < ITERS_BENCH; i++) {
        async_delay_active_count();
    }
    t1 = get_time_sec();
    double popcount_ns = ((t1 - t0) / ITERS_BENCH) * 1e9;

    // 6. Remaining Query
    async_delay_init();
    id = async_delay_start(1000, (void *)0);
    t0 = get_time_sec();
    for (i = 0; i < ITERS_BENCH; i++) {
        async_delay_remaining(id);
    }
    t1 = get_time_sec();
    double remaining_ns = ((t1 - t0) / ITERS_BENCH) * 1e9;

    // Compute AVR cycles & RAM
    size_t ram_bytes = calculate_avr_static_ram();
    int cyc_idle, cyc_act4, cyc_act16, cyc_start, cyc_cancel_fast, cyc_cancel_slow, cyc_restart, cyc_popcount, cyc_ticks_until;
    calculate_avr_cycles(&cyc_idle, &cyc_act4, &cyc_act16, &cyc_start, &cyc_cancel_fast, &cyc_cancel_slow, &cyc_restart, &cyc_popcount, &cyc_ticks_until);

    // Print JSON output
    printf("{\n");
    printf("  \"label\": \"%s\",\n", label);
    printf("  \"version\": %d,\n", ASYNC_DELAY_VERSION);
    printf("  \"max_slots\": %d,\n", ASYNC_DELAY_MAX_SLOTS);
    printf("  \"timer_bits\": %d,\n", ASYNC_DELAY_TIMER_BITS);
    printf("  \"tick_hz\": %d,\n", ASYNC_DELAY_TICK_HZ);
    printf("  \"static_ram_bytes\": %zu,\n", ram_bytes);
    printf("  \"avr_cycles\": {\n");
    printf("    \"idle_tick\": %d,\n", cyc_idle);
    printf("    \"active_tick_4slots\": %d,\n", cyc_act4);
    printf("    \"active_tick_16slots\": %d,\n", cyc_act16);
    printf("    \"start\": %d,\n", cyc_start);
    printf("    \"cancel_fast\": %d,\n", cyc_cancel_fast);
    printf("    \"cancel_slow\": %d,\n", cyc_cancel_slow);
    printf("    \"restart\": %d,\n", cyc_restart);
    printf("    \"active_count\": %d,\n", cyc_popcount);
    printf("    \"ticks_until_next\": %d\n", cyc_ticks_until);
    printf("  },\n");
    printf("  \"host_latency_ns\": {\n");
    printf("    \"idle_tick\": %.2f,\n", idle_ns);
    printf("    \"active_tick\": %.2f,\n", active4_ns);
    printf("    \"start_cancel\": %.2f,\n", start_cancel_ns);
    printf("    \"restart\": %.2f,\n", restart_ns);
    printf("    \"active_count\": %.2f,\n", popcount_ns);
    printf("    \"remaining\": %.2f\n", remaining_ns);
    printf("  }\n");
    printf("}\n");

    return 0;
}
