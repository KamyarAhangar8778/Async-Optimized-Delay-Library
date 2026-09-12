// test_async_delay.c - Host-based unit test suite for async_delay.h
// Compiles natively with GCC/Clang on Linux, macOS, or Windows.
//
// Exercises:
//   - Initialization and reset
//   - One-shot polling delays and elapsed() lifecycle
//   - One-shot and periodic callback execution
//   - Deferred callback execution via async_delay_poll()
//   - In-place retargeting via async_delay_restart()
//   - Fast-path cancellation and async_delay_cancel_all()
//   - Counter wrap-around boundary arithmetic
//   - Power-saving / Sleep helpers: ticks_until_next, remaining, active_count
//   - Slot allocation limits (ASYNC_DELAY_NO_SLOT)
//   - LUT bitmask validation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef __AVR__
static volatile unsigned char TCCR1A = 0;
static volatile unsigned char TCCR1B = 0;
static volatile unsigned char TCCR2 = 0;
static volatile unsigned char TCNT1H = 0;
static volatile unsigned char TCNT1L = 0;
static volatile unsigned char TCNT2 = 0;
static volatile unsigned char OCR1AH = 0;
static volatile unsigned char OCR1AL = 0;
static volatile unsigned char OCR2 = 0;
static volatile unsigned char TIMSK = 0;
#endif

#ifndef ASYNC_DELAY_TICK_HZ
#define ASYNC_DELAY_TICK_HZ 1000
#endif

#define ASYNC_DELAY_TEST_HARDWARE_MACROS 1
#include "../async_delay.h"

static int test_total = 0;
static int test_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    test_total++; \
    if (cond) { \
        test_passed++; \
    } else { \
        printf("  [FAIL] Line %d: %s\n", __LINE__, msg); \
    } \
} while (0)

// ---- Test Callbacks ----
#if !ASYNC_DELAY_DISABLE_CALLBACKS
static volatile unsigned int cb_count1 = 0;
static volatile unsigned int cb_count2 = 0;
static volatile unsigned char last_cb_slot = 0xFF;

static void test_cb_oneshot(unsigned char slot_id)
{
    cb_count1++;
    last_cb_slot = slot_id;
}

static void test_cb_periodic(unsigned char slot_id)
{
    (void)slot_id;
    cb_count2++;
}
#endif

// Simulate hardware timer ticks
static void simulate_ticks(unsigned int count)
{
    unsigned int k;
    for (k = 0; k < count; k++)
    {
        async_delay_tick();
    }
}

// -------------------------------------------------------------
// Test 1: Initialization
// -------------------------------------------------------------
static void test_init(void)
{
    printf("Running test_init...\n");
    async_delay_init();
    TEST_ASSERT(async_delay_active_count() == 0, "active count is 0 after init");
    TEST_ASSERT(async_delay_ticks_until_next() == 0, "ticks until next is 0 after init");
}

// -------------------------------------------------------------
// Test 2: One-shot Polling & Power-Saving Helpers
// -------------------------------------------------------------
static void test_oneshot_polling(void)
{
    unsigned char id;
    printf("Running test_oneshot_polling...\n");
    async_delay_init();

    id = async_delay_start(50, (void *)0);
    TEST_ASSERT(id != ASYNC_DELAY_NO_SLOT, "slot allocated successfully");
    TEST_ASSERT(async_delay_is_active(id) == 1, "slot is active");
    TEST_ASSERT(async_delay_active_count() == 1, "active count is 1");
    TEST_ASSERT(async_delay_remaining(id) == 50, "remaining ticks is 50");
    TEST_ASSERT(async_delay_ticks_until_next() == 50, "ticks until next is 50");

    simulate_ticks(20);
    TEST_ASSERT(async_delay_remaining(id) == 30, "remaining ticks is 30 after 20 ticks");
    TEST_ASSERT(async_delay_ticks_until_next() == 30, "ticks until next is 30 after 20 ticks");
    TEST_ASSERT(async_delay_elapsed(id) == 0, "not elapsed at 20 ticks");

    simulate_ticks(30); // Reach 50 ticks
    TEST_ASSERT(async_delay_is_active(id) == 0, "slot is no longer active at 50 ticks");
    TEST_ASSERT(async_delay_remaining(id) == 0, "remaining ticks is 0");
    TEST_ASSERT(async_delay_elapsed(id) == 1, "elapsed returns 1 at 50 ticks");
    TEST_ASSERT(async_delay_elapsed(id) == 0, "second call to elapsed returns 0 (freed)");
    TEST_ASSERT(async_delay_active_count() == 0, "active count is 0 after polling complete");
}

#if !ASYNC_DELAY_DISABLE_CALLBACKS
// -------------------------------------------------------------
// Test 3: One-shot Callback
// -------------------------------------------------------------
static void test_oneshot_callback(void)
{
    unsigned char id;
    printf("Running test_oneshot_callback...\n");
    async_delay_init();
    cb_count1 = 0;

    id = async_delay_start(40, test_cb_oneshot);
    TEST_ASSERT(id != ASYNC_DELAY_NO_SLOT, "callback slot allocated");

    simulate_ticks(39);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count1 == 0, "callback not fired at 39 ticks");

    simulate_ticks(1);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count1 == 1, "callback fired at 40 ticks");
    TEST_ASSERT(last_cb_slot == id, "callback received correct slot id");

    simulate_ticks(20);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count1 == 1, "callback did not fire again");
    TEST_ASSERT(async_delay_active_count() == 0, "slot automatically freed");
}

#if !ASYNC_DELAY_DISABLE_PERIODIC
// -------------------------------------------------------------
// Test 4: Periodic Callback
// -------------------------------------------------------------
static void test_periodic_callback(void)
{
    unsigned char id;
    printf("Running test_periodic_callback...\n");
    async_delay_init();
    cb_count2 = 0;

    id = async_delay_start_periodic(25, test_cb_periodic);
    TEST_ASSERT(id != ASYNC_DELAY_NO_SLOT, "periodic slot allocated");

    simulate_ticks(24);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count2 == 0, "periodic callback not fired at 24 ticks");

    simulate_ticks(1); // 25
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count2 == 1, "periodic fired at 25 ticks");

    simulate_ticks(25); // 50
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count2 == 2, "periodic fired at 50 ticks");

    simulate_ticks(25); // 75
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count2 == 3, "periodic fired at 75 ticks");

    async_delay_cancel(id);
    TEST_ASSERT(async_delay_active_count() == 0, "periodic slot cancelled");

    simulate_ticks(50);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    async_delay_poll();
#endif
    TEST_ASSERT(cb_count2 == 3, "periodic does not fire after cancel");
}
#endif
#endif

// -------------------------------------------------------------
// Test 5: Cancellation & Cancel All
// -------------------------------------------------------------
static void test_cancel_and_cancel_all(void)
{
    unsigned char id1, id2, id3;
    printf("Running test_cancel_and_cancel_all...\n");
    async_delay_init();

    id1 = async_delay_start(40, (void *)0);
    id2 = async_delay_start(60, (void *)0);
    id3 = async_delay_start(80, (void *)0);

    TEST_ASSERT(async_delay_active_count() == 3, "3 slots active");
    async_delay_cancel(id2);
    TEST_ASSERT(async_delay_active_count() == 2, "2 slots active after single cancel");
    TEST_ASSERT(async_delay_is_active(id2) == 0, "id2 is inactive");
    TEST_ASSERT(async_delay_is_active(id1) == 1, "id1 is still active");
    TEST_ASSERT(async_delay_is_active(id3) == 1, "id3 is still active");

    async_delay_cancel_all();
    TEST_ASSERT(async_delay_active_count() == 0, "0 slots active after cancel_all");
    TEST_ASSERT(async_delay_is_active(id1) == 0, "id1 is inactive");
    TEST_ASSERT(async_delay_is_active(id3) == 0, "id3 is inactive");
}

#if ASYNC_DELAY_FEATURE_RESTART
// -------------------------------------------------------------
// Test 6: Restart API
// -------------------------------------------------------------
static void test_restart(void)
{
    unsigned char id, ok;
    printf("Running test_restart...\n");
    async_delay_init();

    id = async_delay_start(100, (void *)0);
    simulate_ticks(40);
    TEST_ASSERT(async_delay_remaining(id) == 60, "remaining is 60 before restart");

    // Retarget slot in-place to 30 ticks from NOW
    ok = async_delay_restart(id, 30);
    TEST_ASSERT(ok == 1, "restart succeeded");
    TEST_ASSERT(async_delay_remaining(id) == 30, "remaining is now 30");

    simulate_ticks(29);
    TEST_ASSERT(async_delay_elapsed(id) == 0, "not elapsed at 29 ticks after restart");

    simulate_ticks(1);
    TEST_ASSERT(async_delay_elapsed(id) == 1, "elapsed at 30 ticks after restart");
}
#endif

// -------------------------------------------------------------
// Test 7: Counter Wrap-Around
// -------------------------------------------------------------
static void test_counter_wrap(void)
{
    unsigned char id;
    printf("Running test_counter_wrap...\n");
    async_delay_init();

    // Force tick counter close to overflow boundary
#if ASYNC_DELAY_TIMER_BITS == 8
    _async_tick_counter = 245; // wraps past 255
#elif ASYNC_DELAY_TIMER_BITS == 16
    _async_tick_counter = 65525; // wraps past 65535
#else
    _async_tick_counter = 4294967285U;
#endif

    // Start a 20-tick delay across the wrap point
    id = async_delay_start(20, (void *)0);
    TEST_ASSERT(id != ASYNC_DELAY_NO_SLOT, "allocated across wrap boundary");
    TEST_ASSERT(async_delay_remaining(id) == 20, "remaining is 20 across wrap boundary");

    simulate_ticks(19);
    TEST_ASSERT(async_delay_elapsed(id) == 0, "not elapsed at 19 ticks across wrap");

    simulate_ticks(1);
    TEST_ASSERT(async_delay_elapsed(id) == 1, "elapsed correctly fires across wrap boundary");
}

// -------------------------------------------------------------
// Test 8: Slot Exhaustion
// -------------------------------------------------------------
static void test_slot_exhaustion(void)
{
    unsigned char i;
    unsigned char ids[ASYNC_DELAY_MAX_SLOTS + 2];
    printf("Running test_slot_exhaustion...\n");
    async_delay_init();

    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        ids[i] = async_delay_start(100, (void *)0);
        TEST_ASSERT(ids[i] != ASYNC_DELAY_NO_SLOT, "slot allocation under limit");
    }

    ids[ASYNC_DELAY_MAX_SLOTS] = async_delay_start(100, (void *)0);
    TEST_ASSERT(ids[ASYNC_DELAY_MAX_SLOTS] == ASYNC_DELAY_NO_SLOT, "allocation beyond max fails");

    async_delay_cancel(ids[0]);
    ids[0] = async_delay_start(100, (void *)0);
    TEST_ASSERT(ids[0] != ASYNC_DELAY_NO_SLOT, "reallocation after free succeeds");

    async_delay_cancel_all();
}

// -------------------------------------------------------------
// Test 9: LUT Bitmask Equivalence
// -------------------------------------------------------------
static void test_lut_bitmask(void)
{
    unsigned char i;
    printf("Running test_lut_bitmask...\n");
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        async_mask_t expected_bit = (async_mask_t)(1 << i);
        async_mask_t expected_clr = (async_mask_t)~(1 << i);
        TEST_ASSERT(_AD_SLOT_BIT(i) == expected_bit, "LUT slot bit matches (1 << i)");
        TEST_ASSERT(_AD_SLOT_CLR(i) == expected_clr, "LUT slot clr matches ~(1 << i)");
    }
}

// -------------------------------------------------------------
// Test 10: O(1) LUT Allocator & Popcount Correctness
// -------------------------------------------------------------
static void test_lut_alloc_and_popcount(void)
{
    unsigned char id0, id1, id2, id3;
    printf("Running test_lut_alloc_and_popcount...\n");
    async_delay_init();

    TEST_ASSERT(async_delay_active_count() == 0, "popcount 0 on empty");

    id0 = async_delay_start(100, (void *)0);
    TEST_ASSERT(id0 == 0, "first allocated slot is 0");
    TEST_ASSERT(async_delay_active_count() == 1, "popcount is 1");

#if ASYNC_DELAY_MAX_SLOTS >= 2
    id1 = async_delay_start(100, (void *)0);
    TEST_ASSERT(id1 == 1, "second allocated slot is 1");
    TEST_ASSERT(async_delay_active_count() == 2, "popcount is 2");
#endif

#if ASYNC_DELAY_MAX_SLOTS >= 4
    id2 = async_delay_start(100, (void *)0);
    id3 = async_delay_start(100, (void *)0);
    TEST_ASSERT(id2 == 2 && id3 == 3, "allocated slots 2 and 3");
    TEST_ASSERT(async_delay_active_count() == 4, "popcount is 4");

    // Free slot 1, next allocation must reuse slot 1 in O(1)
    async_delay_cancel(id1);
    TEST_ASSERT(async_delay_active_count() == 3, "popcount is 3 after cancel slot 1");
    id1 = async_delay_start(100, (void *)0);
    TEST_ASSERT(id1 == 1, "LUT allocator immediately reused freed slot 1");
    TEST_ASSERT(async_delay_active_count() == 4, "popcount back to 4");
#endif

    async_delay_cancel_all();
    TEST_ASSERT(async_delay_active_count() == 0, "popcount 0 after cancel all");
}

static void test_hardware_timer_setup(void)
{
    printf("Running test_hardware_timer_setup...\n");
    TCCR2 = 0; OCR2 = 0; TIMSK = 0;
    ASYNC_DELAY_SETUP_TIMER2_CTC_8MHZ_1KHZ();
    TEST_ASSERT(TCCR2 == 0x0C, "Timer2 CTC 8MHz prescaler is /64 (0x0C)");
    TEST_ASSERT(OCR2 == 124, "Timer2 CTC 8MHz OCR2 is 124");
    TEST_ASSERT((TIMSK & 0x80) != 0, "Timer2 OCIE2 enabled");

    TCCR2 = 0; OCR2 = 0; TIMSK = 0;
    ASYNC_DELAY_SETUP_TIMER2_CTC_16MHZ_1KHZ();
    TEST_ASSERT(TCCR2 == 0x0C, "Timer2 CTC 16MHz prescaler is /64 (0x0C)");
    TEST_ASSERT(OCR2 == 249, "Timer2 CTC 16MHz OCR2 is 249");

    TCCR2 = 0; OCR2 = 0; TIMSK = 0;
    ASYNC_DELAY_SETUP_TIMER2_CTC_4MHZ_1KHZ();
    TEST_ASSERT(TCCR2 == 0x0B, "Timer2 CTC 4MHz prescaler is /32 (0x0B)");
    TEST_ASSERT(OCR2 == 124, "Timer2 CTC 4MHz OCR2 is 124");

    TCCR2 = 0; OCR2 = 0; TIMSK = 0;
    ASYNC_DELAY_SETUP_TIMER2_CTC_2MHZ_1KHZ();
    TEST_ASSERT(TCCR2 == 0x0A, "Timer2 CTC 2MHz prescaler is /8 (0x0A)");
    TEST_ASSERT(OCR2 == 249, "Timer2 CTC 2MHz OCR2 is 249");

    TCCR2 = 0; OCR2 = 0; TIMSK = 0;
    ASYNC_DELAY_SETUP_TIMER2_CTC_1MHZ_1KHZ();
    TEST_ASSERT(TCCR2 == 0x0A, "Timer2 CTC 1MHz prescaler is /8 (0x0A)");
    TEST_ASSERT(OCR2 == 124, "Timer2 CTC 1MHz OCR2 is 124");

    TCCR1A = 0; TCCR1B = 0; OCR1AH = 0; OCR1AL = 0; TIMSK = 0;
    ASYNC_DELAY_SETUP_TIMER1_CTC_8MHZ_1KHZ();
    TEST_ASSERT(TCCR1B == 0x0A, "Timer1 CTC 8MHz prescaler is /8 (0x0A)");
    TEST_ASSERT(OCR1AH == 0x03 && OCR1AL == 0xE7, "Timer1 CTC 8MHz OCR is 999 (0x03E7)");

    async_delay_hw_timer1_init(16000000UL, 1000);
    TEST_ASSERT(OCR1AH == 0x07 && OCR1AL == 0xCF, "Timer1 hw init 16MHz/1kHz OCR is 1999 (0x07CF)");

    TCCR2 = 0; OCR2 = 0; TIMSK = 0;
    async_delay_hw_timer2_init(8000000UL, 1000);
    TEST_ASSERT(TCCR2 == 0x0B, "Timer2 hw init 8MHz/1kHz prescaler is /32 (0x0B)");
    TEST_ASSERT(OCR2 == 249, "Timer2 hw init 8MHz/1kHz OCR2 is 249");
}

static void test_duration_boundary_limits(void)
{
    unsigned char id;
    printf("Running test_duration_boundary_limits...\n");
    async_delay_init();

    // Oversized duration (> _ASYNC_HALF_RANGE) must fail
    id = async_delay_start((async_tick_t)(_ASYNC_HALF_RANGE + 1), (void *)0);
    TEST_ASSERT(id == ASYNC_DELAY_NO_SLOT, "start fails for duration > half_range");

    // Exact half range must succeed
    id = async_delay_start(_ASYNC_HALF_RANGE, (void *)0);
    TEST_ASSERT(id != ASYNC_DELAY_NO_SLOT, "start succeeds for duration == half_range");

#if ASYNC_DELAY_FEATURE_RESTART
    TEST_ASSERT(async_delay_restart(id, (async_tick_t)(_ASYNC_HALF_RANGE + 1)) == 0, "restart fails for new_duration > half_range");
    TEST_ASSERT(async_delay_restart(id, 100) == 1, "restart succeeds for valid duration");
#endif

    async_delay_cancel(id);
}

// =============================================================
int main(void)
{
    printf("========================================\n");
    printf("  async_delay Host Unit Test Suite\n");
    printf("  Config: BITS=%d, SLOTS=%d, DEFERRED=%d\n",
           ASYNC_DELAY_TIMER_BITS, ASYNC_DELAY_MAX_SLOTS, ASYNC_DELAY_DEFERRED_CALLBACKS);
    printf("========================================\n");

    test_init();
    test_oneshot_polling();
#if !ASYNC_DELAY_DISABLE_CALLBACKS
    test_oneshot_callback();
#if !ASYNC_DELAY_DISABLE_PERIODIC
    test_periodic_callback();
#endif
#endif
    test_cancel_and_cancel_all();
#if ASYNC_DELAY_FEATURE_RESTART
    test_restart();
#endif
    test_counter_wrap();
    test_slot_exhaustion();
    test_lut_bitmask();
    test_lut_alloc_and_popcount();
    test_hardware_timer_setup();
    test_duration_boundary_limits();

    printf("\n----------------------------------------\n");
    printf("Tests Run: %d | Passed: %d | Failed: %d\n",
           test_total, test_passed, test_total - test_passed);
    printf("----------------------------------------\n");

    if (test_passed == test_total)
    {
        printf(">>> ALL TESTS PASSED! <<<\n\n");
        return 0;
    }
    else
    {
        printf(">>> SOME TESTS FAILED! <<<\n\n");
        return 1;
    }
}
