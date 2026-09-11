// benchmark_compare.c - Automated baseline vs optimized performance comparison
// Compares current active optimizations against a baseline (unoptimized/naive mode)
// and displays exact speedup ratios, latency reductions, and regressions.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef ASYNC_DELAY_TICK_HZ
#define ASYNC_DELAY_TICK_HZ 1000
#endif

#include "../async_delay.h"

#define BENCH_ITERS 2000000

static double get_time_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[])
{
    double t0, t1;
    unsigned long i;
    unsigned char id;
    const char *label = (argc > 1) ? argv[1] : "CURRENT";

    // 1. Idle Tick Benchmark (0 active slots)
    async_delay_init();
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        async_delay_tick();
    }
    t1 = get_time_sec();
    double idle_ns = ((t1 - t0) / BENCH_ITERS) * 1e9;
    double idle_mops = (BENCH_ITERS / (t1 - t0)) / 1e6;

    // 2. Active Tick (all slots active, none due)
    async_delay_init();
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        async_delay_start(30000, (void *)0);
    }
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        async_delay_tick();
    }
    t1 = get_time_sec();
    double active_ns = ((t1 - t0) / BENCH_ITERS) * 1e9;
    double active_mops = (BENCH_ITERS / (t1 - t0)) / 1e6;

    // 3. Start + Cancel Cycle Benchmark
    async_delay_init();
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        id = async_delay_start(100, (void *)0);
        async_delay_cancel(id);
    }
    t1 = get_time_sec();
    double start_cancel_ns = ((t1 - t0) / BENCH_ITERS) * 1e9;
    double start_cancel_mops = (BENCH_ITERS / (t1 - t0)) / 1e6;

    // 4. In-Place Restart Benchmark
    async_delay_init();
    id = async_delay_start(1000, (void *)0);
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
#if ASYNC_DELAY_FEATURE_RESTART
        async_delay_restart(id, 1000);
#endif
    }
    t1 = get_time_sec();
    double restart_ns = ((t1 - t0) / BENCH_ITERS) * 1e9;
    double restart_mops = (BENCH_ITERS / (t1 - t0)) / 1e6;

    // 5. Active Count Query Benchmark
    async_delay_init();
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS / 2 + 1; i++)
    {
        async_delay_start(1000, (void *)0);
    }
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        async_delay_active_count();
    }
    t1 = get_time_sec();
    double popcount_ns = ((t1 - t0) / BENCH_ITERS) * 1e9;
    double popcount_mops = (BENCH_ITERS / (t1 - t0)) / 1e6;

    // Format output: CSV line if requested, or readable line
    if (argc > 2 && argv[2][0] == 'c')
    {
        printf("%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
               label,
               idle_ns, idle_mops,
               active_ns, active_mops,
               start_cancel_ns, start_cancel_mops,
               restart_ns, restart_mops,
               popcount_ns, popcount_mops);
    }
    else
    {
        printf("  %-25s : %7.2f M ops/sec (%5.2f ns/op)\n", "Idle tick (0 active)", idle_mops, idle_ns);
        printf("  %-25s : %7.2f M ops/sec (%5.2f ns/op)\n", "Active tick (in-flight)", active_mops, active_ns);
        printf("  %-25s : %7.2f M ops/sec (%5.2f ns/op)\n", "Start + Cancel pair", start_cancel_mops, start_cancel_ns);
        printf("  %-25s : %7.2f M ops/sec (%5.2f ns/op)\n", "async_delay_restart()", restart_mops, restart_ns);
        printf("  %-25s : %7.2f M ops/sec (%5.2f ns/op)\n", "async_delay_active_count()", popcount_mops, popcount_ns);
    }

    return 0;
}
