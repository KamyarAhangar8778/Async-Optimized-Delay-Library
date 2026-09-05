// test_gate.c - T15: next-target early-return gate (plan 006 §4.4).
// Included by test_main.c (which supplies main). NOT part of the firmware.
//
// T16 (differential equivalence) is NOT a separate test: the harness runs
// T1-T15 under EVERY combo in make_host.py's matrix and asserts identical
// callback sequences / fire ticks (modulo T8/T14 which are combo-conditional
// by #if). Turning any OPT_* off must not change observable behavior.
#include "test_common.h"

// ---------- T15: next-target gate - active but nothing due ----------
// Fill every slot with a long delay, none due: 50 ticks must produce zero
// fires while the driver's counter stays exact. Proves the early-return path
// (OPT_NEXT_TARGET / idle gate) neither stalls the counter nor drops a
// pending expiry. Slot-count agnostic: 1..8 all exercise the same gate.
static void t_gate_idle(void)
{
    unsigned char i;
    unsigned char n_fired_expected;
    g_case = "T15 gate";
    async_delay_init();
    t_reset();
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        async_delay_start(T_LONG, cb_a);
    ticks(50);
    maybe_poll();
    T_EQ("zero fires while nothing due", 0, g_ev_n);
    T_EQ("counter exact after gated ticks", 50, g_now);
    // And the gate must not EAT the expiry: run to the due point, all fire.
    ticks(T_LONG - 50);
    maybe_poll();
    n_fired_expected = (unsigned char)ASYNC_DELAY_MAX_SLOTS;
    T_EQ("all fire once due", n_fired_expected, g_ev_n);
}

static void run_gate_tests(void)
{
    t_gate_idle();
}
