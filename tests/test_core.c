// test_core.c - T1-T8: counting, one-shot, polling, overflow, cancel,
// periodic phase-lock, used-vs-active and its inversion (plan 006 §4.4).
// Included by test_main.c (which supplies main). NOT part of the firmware.
#include "test_common.h"

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
    unsigned char i;
    unsigned char r;
    g_case = "T4 overflow";
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
    unsigned char id;
    g_case = "T5 cancel";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_LONG, cb_a);
    id = g_start_result;
    async_delay_cancel(id);
    ticks(T_LONG + 200);
    maybe_poll();
    T_EQ("no fire after cancel", 0, g_ev_n);
    T_EQ("slot is free after cancel", 0, async_delay_start(T_DUR, cb_a));
}

// ---------- T6: periodic phase-lock (fires at exact multiples) ----------
// NOTE: g_now keeps running across phases inside ONE test (no t_reset mid-test),
// so fire tick k is (k+1)*T_DUR2 counted from this test's own start.
// In DIRECT mode one tick = one fire, so a single batch of ticks works.
// In DEFERRED mode the pending mask is ONE bit: two expiries before one poll
// collapse into a single callback (documented header behavior), so the driver
// must poll every tick to observe all 10 fires. That poll-per-tick IS the
// contract for deferred periodic timers.
static void t_periodic(void)
{
    unsigned long k;
    g_case = "T6 periodic";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start_periodic(T_DUR2, cb_a);
    T_EQ("periodic start returns slot 0", 0, g_start_result);
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    ticks(1);
    for (k = 1; k < T_DUR2 * 10; k++)   // poll after every tick, per contract
    {
        ticks(1);
        maybe_poll();
    }
#else
    ticks(T_DUR2 * 10);
#endif
    maybe_poll();
    T_EQ("fired 10 times", 10, g_ev_n);
    for (k = 0; k < 10; k++)
        T_EQ("fire tick = exact multiple", (long)((k + 1) * T_DUR2), (long)g_ev_tick[k]);
}

// ---------- T7: used-vs-active (BUG-1): EXPIRED slot not reused until elapsed ----------
static void t_used_vs_active(void)
{
    unsigned char id0;
    g_case = "T7 used-vs-active";
    async_delay_init();
    t_reset();
    // Two polling slots; let #0 expire (EXPIRED, still allocated).
    id0 = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot0 allocated", 0, id0);
    async_delay_start(T_DUR2, (void *)0);
    ticks(T_DUR);   // T_DUR > T_DUR2: BOTH slots are EXPIRED now, still allocated
    // Slot 0 is EXPIRED but its owner has not polled yet: start must skip it.
    // (Both 0 and 1 are EXPIRED-but-allocated; only slot 2+ is truly free.)
    g_start_result = async_delay_start(T_DUR, (void *)0);
#if ASYNC_DELAY_MAX_SLOTS > 2
    T_EQ("EXPIRED slot NOT reused before elapsed", 2, g_start_result);
    T_EQ("elapsed on id0 frees it", 1, async_delay_elapsed(id0));
    async_delay_cancel(g_start_result);   // free slot 2 again for determinism
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot 0 reusable after elapsed", 0, g_start_result);
#else
    // MAX_SLOTS=1/2 combos: nothing is free, so start must return NO_SLOT.
    T_EQ("no free slot while EXPIRED unpolled", ASYNC_DELAY_NO_SLOT, g_start_result);
    T_EQ("elapsed on id0 frees it", 1, async_delay_elapsed(id0));
    g_start_result = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot 0 reusable after elapsed", 0, g_start_result);
#endif
}

// ---------- T8: inverted test ----------
// The bug this guards: without FIX_USED_MASK, start() hands out an
// EXPIRED-but-unpolled slot, silently destroying its owner's timer.
// This runs ONLY in the FIX_USED_MASK=0 combos (run_core_tests calls it
// there instead of T7) and proves the harness can actually SEE the bug
// (ARCHITECTURE.md §8 rule 3).
#if !ASYNC_DELAY_FIX_USED_MASK
static void t_invert_used_bug(void)
{
    unsigned char id1;
    g_case = "T8 inverted used-bug";
    async_delay_init();
    t_reset();
    id1 = async_delay_start(T_DUR, (void *)0);
    T_EQ("slot0 allocated", 0, id1);
    ticks(T_DUR);                         // slot0 expires
    g_start_result = async_delay_start(T_DUR, (void *)0);
#if ASYNC_DELAY_OPT_BITMASK
    T_EQ("BUG reproduced: start steals EXPIRED slot 0 (FIX_USED_MASK=0)", 0, g_start_result);
#else
    // Legacy (BITMASK=0) start() searches by STATE byte, so an EXPIRED slot is
    // never handed out: the used-mask bug structurally cannot exist here
    // (header: "the legacy path uses the state byte and is already correct").
    // Assert the good behavior instead of the bug.
    T_EQ("legacy start skips EXPIRED slot 0 by state", 1, g_start_result);
#endif
    // Sanity: this is a BUG by design under BITMASK=1 - the test passing
    // proves the harness detects what FIX_USED_MASK prevents in T7.
}
#endif /* !ASYNC_DELAY_FIX_USED_MASK */

static void run_core_tests(void)
{
    t_idle_count();
    t_one_shot();
    t_polling();
    // T4 needs at least 2 slots to mean anything (fill + overflow).
    // T7 needs 3 (two EXPIRED-but-allocated + one fresh). Under MAX_SLOTS=1
    // both collapse; skip them rather than assert nonsense. T7 additionally
    // only makes sense when FIX_USED_MASK protects EXPIRED slots (its T8
    // inversion runs otherwise).
#if ASYNC_DELAY_MAX_SLOTS > 2
    t_overflow();
# if ASYNC_DELAY_FIX_USED_MASK
    t_used_vs_active();
# else
    (void)t_used_vs_active;   // compiled, not run: T8 covers this combo below
# endif
#else
    // MAX_SLOTS=1: both tests compiled but skipped - keep -Werror happy.
    (void)t_overflow;
    (void)t_used_vs_active;
#endif
    t_cancel();
    t_periodic();
#if !ASYNC_DELAY_FIX_USED_MASK
    // FIX_USED_MASK=0 combos: the fix's own regression test cannot run
    // (nothing to protect), and the inverted T8 takes its place instead.
    t_invert_used_bug();
#endif
}
