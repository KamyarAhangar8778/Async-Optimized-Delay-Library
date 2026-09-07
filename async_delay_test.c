/*******************************************************
async_delay Library Test Project
Chip type           : ATmega8
Clock frequency     : 8.000000 MHz (internal RC oscillator)
// KEEP this at 8 MHz: Timer2 OCR2 = 124 below is calculated for 125 kHz.
// If you change the clock, recompute OCR2 and update async_delay_test.cwp CPUClock.

Tests the async_delay library:
  - Timer2 CTC generates 1ms ticks
  - LED on PB0 blinks every 500ms via DEFERRED callback (drained by
    async_delay_poll() in the main loop - DEFERRED_CALLBACKS=1)
  - LED on PB1 blinks every 750ms via polling
  - Cancel test (2000ms delay cancelled early -> never fires)
  - Slot overflow test (5th start must return 0xFF)
  - LCD refresh paced by async_delay every 200ms
  - loop_count (unsigned long) proves main loop is NOT blocked
*******************************************************/

#include <mega8.h>
#include <alcd.h>
#include <delay.h>

// ---------- async_delay configuration ----------
#define ASYNC_DELAY_TIMER_BITS  16
#define ASYNC_DELAY_MAX_SLOTS   4
#define ASYNC_DELAY_TICK_HZ     1000   // 1ms tick
#define ASYNC_DELAY_DEFERRED_CALLBACKS 1  // callbacks run from async_delay_poll() in main loop

#include <async_delay.h>

// Timer2 at 8 MHz: prescaler /64 -> timer clock = 125 kHz
// 1 ms = 125 ticks -> OCR2 = 125 - 1 = 124
#define OCR2_1MS  124

// ---------- Test state variables ----------
static unsigned char led0_toggle = 0;      // set by callback (PB0)
static unsigned char led1_toggle = 0;      // set by polling (PB1)
static unsigned char test_cancel_done = 0;
static unsigned char test_cancel_failed = 0;
static unsigned char test_overflow_done = 0;
static unsigned long  loop_count = 0;      // 32-bit: never wraps at 60000

// ---------- Callback: toggle LED0 flag (deferred: runs from async_delay_poll
// in MAIN context, so it may do anything - still kept short as good practice) ----------
void cb_led0_toggle(unsigned char slot_id)
{
    (void)slot_id;       // parameter unused — silence warning
    led0_toggle ^= 1;
}

// ---------- Callback for cancel test: should NEVER fire ----------
void cb_should_not_fire(unsigned char slot_id)
{
    // Just set a flag - do NOT touch the LCD from an ISR
    (void)slot_id;       // parameter unused — silence warning
    test_cancel_failed = 1;
}

// ---------- Helper: unsigned long to right-aligned 10-char string ----------
void ulong_to_str(unsigned long val, char *buf)
{
    unsigned char i, n = 0;
    char tmp[11];
    do { tmp[n++] = '0' + (val % 10); val /= 10; } while (val);
    for (i = 0; i < 10 - n; i++) buf[i] = ' ';
    for (i = 0; i < n; i++) buf[10 - n + i] = tmp[n - 1 - i];
    buf[10] = '\0';
}

// ============================================================
// Timer2 Compare Match ISR - generates 1ms ticks
// ============================================================
interrupt [TIM2_COMP] void timer2_comp_isr(void)
{
    async_delay_tick();
}

// ============================================================
void main(void)
{
    unsigned char id_poll, id_cancel, id_lcd, id_fail;
    char num_buf[11];

    // ---- Port B init: PB0 and PB1 as output (LEDs) ----
    DDRB = (1 << DDB1) | (1 << DDB0);
    PORTB = 0x00;

    // ---- Timer2 init: CTC mode, 1ms interrupt ----
    // 8 MHz / 64 = 125 kHz, OCR2 = 124 -> period = 1ms
    ASSR = 0x00;
    TCCR2 = (1 << WGM21)                          // CTC mode
          | (0 << COM21) | (0 << COM20)           // OC2 disconnected
          | (1 << CS22) | (0 << CS21) | (0 << CS20);  // prescaler /64
    TCNT2 = 0x00;
    OCR2  = OCR2_1MS;
    TIMSK |= (1 << OCIE2);                        // enable Timer2 compare ISR

    // ---- LCD init + boot banner ----
    lcd_init(16);
    lcd_clear();
    lcd_gotoxy(0, 0);
    lcd_putsf("AsyncDelay Test");
    lcd_gotoxy(0, 1);
    lcd_putsf("Initializing...");
    // One-time boot pause so the banner is visible.
    // This is NOT in the main loop - runtime is fully non-blocking.
    delay_ms(2000);
    lcd_clear();

    // ---- Initialize async_delay ----
    async_delay_init();

    // Enable global interrupts AFTER all init
    #asm("sei")

    // ========================================
    // TEST 1: Periodic callback mode - LED0 toggles every 500ms
    // ========================================
    async_delay_start_periodic(500, cb_led0_toggle);

    // ========================================
    // TEST 2: Polling mode - LED1 toggles every 750ms
    // ========================================
    id_poll = async_delay_start(750, (void *)0);

    // ========================================
    // TEST 3: Cancel test - 2000ms delay, cancelled early
    // ========================================
    id_cancel = async_delay_start(2000, cb_should_not_fire);

    // ========================================
    // TEST 4: LCD pacing - refresh every 200ms via async_delay
    // ========================================
    id_lcd = async_delay_start(200, (void *)0);

    // ========================================
    // TEST 5: Slot overflow - all 4 slots full, 5th must fail
    // ========================================
    id_fail = async_delay_start(100, (void *)0);
    if (id_fail == ASYNC_DELAY_NO_SLOT)
        test_overflow_done = 1;   // PASS
    else
        test_overflow_done = 2;   // FAIL

    // ========================================
    // Main loop - NON-BLOCKING, runs freely
    // ========================================
    while (1)
    {
        async_delay_poll();   // drain deferred callbacks (LED0) - MUST be first

        loop_count++;

        // ---- Apply LED0 state from callback ----
        PORTB.0 = led0_toggle ? 1 : 0;

        // ---- Poll LED1 ----
        if (async_delay_elapsed(id_poll))
        {
            led1_toggle ^= 1;
            PORTB.1 = led1_toggle;
            id_poll = async_delay_start(750, (void *)0);
        }

        // ---- Cancel test: cancel after first few loop iterations ----
        if (!test_cancel_done && loop_count > 100)
        {
            async_delay_cancel(id_cancel);
            test_cancel_done = 1;
        }

        // ---- LCD refresh paced at 200ms real time ----
        if (async_delay_elapsed(id_lcd))
        {
            id_lcd = async_delay_start(200, (void *)0);

            lcd_gotoxy(0, 0);
            lcd_putsf("Loop:");
            ulong_to_str(loop_count, num_buf);
            lcd_puts(num_buf);

            lcd_gotoxy(0, 1);
            if (test_overflow_done == 1)
                lcd_putsf("Ov:OK   ");
            else if (test_overflow_done == 2)
                lcd_putsf("Ov:FAIL ");
            else
                lcd_putsf("Ov:.... ");

            if (test_cancel_failed)
                lcd_putsf("Cnc:FAIL");
            else if (test_cancel_done)
                lcd_putsf("Cnc:OK  ");
            else
                lcd_putsf("Cnc:..  ");
        }
    }
}
