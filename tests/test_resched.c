// test_resched.c - T9-T14: callback self-reschedule, next-target gate,
// cancel-of-minimum, wrap, duration-0, deferred drain (plan 006 §4.4).
// Included by test_main.c (which supplies main). NOT part of the firmware.
#include "test_common.h"

// A callback that re-starts a one-shot (copy the returned id into the
// g_self_slot global from test_common.h so the caller can check it).
static void cb_self01(unsigned char slot_id)
{
    unsigned char r;
    t_record(slot_id);
    r = async_delay_start(T_DUR, cb_a);
    g_self_slot[slot_id] = r;          // where did the new delay land?
    g_self_start_at[slot_id] = g_now;  // when
}

// A periodic callback that starts a one-shot: it must land in a DIFFERENT
// slot because the periodic slot is still ACTIVE during its own callback.
// The landing slot is recorded in g_per_self_slot (test_common.h).
static void cb_per_self(unsigned char slot_id)
{
    t_record(slot_id);
    g_per_self_slot = async_delay_start(T_DUR2, cb_b);
}

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
#if ASYNC_DELAY_CALLBACK_RESCHEDULE
    // RESCHEDULE=1 frees the one-shot slot BEFORE the callback, so the re-start
    // should land on the SAME slot 0 (g_self_slot[0]==0) and fire T_DUR later.
    T_EQ("self-reschedule reused slot 0", 0, g_self_slot[0]);
    ticks(T_DUR);
    maybe_poll();
    T_EQ("self-rescheduled delay fired", 2, g_ev_n);
    T_EQ("re-fired on slot 0", 0, g_ev_slot[1]);
#else
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    // DEFERRED + RESCHEDULE=0: the one-shot slot is freed at EXPIRY time
    // (inside the tick, header _async_delay_expire_slot legacy branch) and the
    // callback only runs later in poll() - so by the time the callback
    // re-starts, slot 0 is already FREE and the re-start reuses it.
    T_EQ("deferred legacy self-reschedule reused slot 0", 0, g_self_slot[0]);
    ticks(T_DUR);
    maybe_poll();
    T_EQ("self-rescheduled delay fired", 2, g_ev_n);
    T_EQ("re-fired on slot 0", 0, g_ev_slot[1]);
#else
    // RESCHEDULE=0 (legacy, direct): the re-start from inside the callback
    // cannot reuse its own still-ACTIVE slot, so it lands on slot 1.
    T_EQ("legacy self-reschedule lands on slot 1", 1, g_self_slot[0]);
    ticks(T_DUR);
    maybe_poll();
    T_EQ("self-rescheduled delay fired", 2, g_ev_n);
    T_EQ("re-fired on slot 1", 1, g_ev_slot[1]);
#endif
#endif
    T_EQ("re-fired T_DUR after first", (long)(g_ev_tick[0] + T_DUR), (long)g_ev_tick[1]);
}

// ---------- T10: periodic self-reschedule lands in a DIFFERENT slot ----------
static void t_periodic_self(void)
{
    g_case = "T10 periodic self";
    if (ASYNC_DELAY_MAX_SLOTS < 2)
        return;   // needs a second slot for the one-shot from the callback
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start_periodic(T_DUR2, cb_per_self);
    T_EQ("periodic start returns slot 0", 0, g_start_result);
    ticks(T_DUR2);    // first periodic fire; callback starts a one-shot
    maybe_poll();
    T_EQ("periodic fired once", 1, g_ev_n);
    T_EQ("one-shot from periodic cb lands on slot 1", 1, g_per_self_slot);
    ticks(T_DUR2);
    maybe_poll();
    // slot 0 fired again (periodic) and slot 1 fired (one-shot): both drained
    // by the same poll in deferred mode.
    T_EQ("periodic + one-shot both fired", 3, g_ev_n);
}

// ---------- T11: cancel-of-minimum keeps the survivor exact ----------
static void t_cancel_min(void)
{
    unsigned char id_far;
    g_case = "T11 cancel-min";
    if (ASYNC_DELAY_MAX_SLOTS < 2)
        return;   // needs two simultaneous slots: victim + survivor
    async_delay_init();
    t_reset();
    async_delay_start(T_DUR, cb_a);           // slot 0, the minimum target
    id_far = async_delay_start(T_LONG, cb_b); // slot 1, survives
    async_delay_cancel(0);                    // kill the minimum
    ticks(T_LONG);
    maybe_poll();
    T_EQ("survivor fired exactly once", 1, g_ev_n);
    T_EQ("survivor fired on slot 1", id_far, g_ev_slot[0]);
    T_EQ("survivor fired exactly at T_LONG", T_LONG, g_ev_tick[0]);
}

// ---------- T12: counter wrap ----------
// The counter is file-static in the library, so the driver cannot poke it.
// Instead tick forward to T_WRAP_BASE and start a delay that crosses the wrap.
static void t_wrap(void)
{
    g_case = "T12 wrap";
    async_delay_init();
    t_reset();
    ticks(T_WRAP_BASE);
    g_start_result = async_delay_start(T_WRAP_DUR, cb_a);
    T_EQ("start returns slot 0", 0, g_start_result);
    ticks(T_WRAP_DUR - 1);
    T_EQ("no fire before wrap-crossing due", 0, g_ev_n);
    maybe_poll();
    ticks(1);
    maybe_poll();
    T_EQ("fired exactly once across wrap", 1, g_ev_n);
    T_EQ("fired at T_WRAP_BASE + T_WRAP_DUR",
         (long)(T_WRAP_BASE + T_WRAP_DUR), (long)g_ev_tick[0]);
}

// ---------- T13: duration 0 fires on the very next tick ----------
static void t_dur0(void)
{
    g_case = "T13 dur0";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(0, cb_a);
    T_EQ("start returns slot 0", 0, g_start_result);
    T_EQ("no fire before any tick", 0, g_ev_n);
    ticks(1);
    maybe_poll();
    T_EQ("fired exactly once after one tick", 1, g_ev_n);
    T_EQ("fired at tick 1", 1, g_ev_tick[0]);
}

// ---------- T14: deferred drain (DEFERRED=1 combos only) ----------
// Proves the deferred contract: after the due tick the callback has NOT run;
// after poll() it has run exactly once.
static void t_deferred(void)
{
#if ASYNC_DELAY_DEFERRED_CALLBACKS
    g_case = "T14 deferred";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_DUR, cb_a);
    T_EQ("start returns slot 0", 0, g_start_result);
    ticks(T_DUR);                       // due - but callbacks stay pending
    T_EQ("callback NOT run before poll", 0, g_ev_n);
    async_delay_poll();
    T_EQ("callback run exactly once after poll", 1, g_ev_n);
    T_EQ("fired on slot 0", 0, g_ev_slot[0]);
    T_EQ("second poll runs nothing new", 1, (async_delay_poll(), g_ev_n));
#else
    g_case = "T14 deferred";
    async_delay_init();
    t_reset();
    g_start_result = async_delay_start(T_DUR, cb_a);
    ticks(T_DUR);                       // direct mode: callback ran in the tick
    T_EQ("direct mode fires without poll", 1, g_ev_n);
#endif
}

static void run_resched_tests(void)
{
    t_self_oneshot();
    t_periodic_self();
    t_cancel_min();
    t_wrap();
    t_dur0();
    t_deferred();
    // Compiled for every combo but only meaningfully run in some; the
    // guards inside each test no-op them where they do not apply.
    (void)t_periodic_self;
    (void)t_cancel_min;
}
