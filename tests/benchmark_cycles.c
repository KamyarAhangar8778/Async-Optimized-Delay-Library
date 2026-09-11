// benchmark_cycles.c - Host-based performance and throughput benchmark
// Measures operations per second and relative CPU cost of async_delay operations.

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

int main(void)
{
    double t0, t1;
    unsigned long i;
    unsigned char id;

    printf("========================================\n");
    printf("  async_delay Throughput Benchmark\n");
    printf("  Iterations per benchmark: %d\n", BENCH_ITERS);
    printf("========================================\n");

    // 1. Idle Tick Benchmark (0 active slots)
    async_delay_init();
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        async_delay_tick();
    }
    t1 = get_time_sec();
    printf("  Idle tick (0 active)    : %7.2f M ops/sec (%.2f ns/op)\n",
           (BENCH_ITERS / (t1 - t0)) / 1e6, ((t1 - t0) / BENCH_ITERS) * 1e9);

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
    printf("  Active tick (%d active)  : %7.2f M ops/sec (%.2f ns/op)\n",
           ASYNC_DELAY_MAX_SLOTS, (BENCH_ITERS / (t1 - t0)) / 1e6, ((t1 - t0) / BENCH_ITERS) * 1e9);

    // 3. Start + Cancel Cycle Benchmark
    async_delay_init();
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        id = async_delay_start(100, (void *)0);
        async_delay_cancel(id);
    }
    t1 = get_time_sec();
    printf("  Start + Cancel pair     : %7.2f M ops/sec (%.2f ns/op)\n",
           (BENCH_ITERS / (t1 - t0)) / 1e6, ((t1 - t0) / BENCH_ITERS) * 1e9);

#if ASYNC_DELAY_FEATURE_RESTART
    // 4. In-Place Restart Benchmark
    async_delay_init();
    id = async_delay_start(1000, (void *)0);
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        async_delay_restart(id, 1000);
    }
    t1 = get_time_sec();
    printf("  async_delay_restart()   : %7.2f M ops/sec (%.2f ns/op)\n",
           (BENCH_ITERS / (t1 - t0)) / 1e6, ((t1 - t0) / BENCH_ITERS) * 1e9);
#endif

    // 5. Remaining / Polling Query Benchmark
    async_delay_init();
    id = async_delay_start(1000, (void *)0);
    t0 = get_time_sec();
    for (i = 0; i < BENCH_ITERS; i++)
    {
        async_delay_remaining(id);
    }
    t1 = get_time_sec();
    printf("  async_delay_remaining() : %7.2f M ops/sec (%.2f ns/op)\n",
           (BENCH_ITERS / (t1 - t0)) / 1e6, ((t1 - t0) / BENCH_ITERS) * 1e9);

    printf("========================================\n");
    return 0;
}
