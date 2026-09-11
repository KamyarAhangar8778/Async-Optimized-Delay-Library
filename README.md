# async_delay.h — Non-Blocking Timer & Delay Library for AVR

`async_delay.h` is a high-performance, header-only, non-blocking asynchronous timer library designed for 8-bit AVR microcontrollers (**ATmega8 / ATmega16 / ATmega32**) compiled with **CodeVisionAVR**.

> **Contract for AI Agents & Firmware Engineers:**
> This document is the definitive integration specification for `async_delay.h`. When incorporating this library into any project, all necessary configuration macros, hardware timer formulas, concurrency constraints, API contracts, CodeVisionAVR quirks, and verified implementation patterns are fully detailed below.

---

## 1. System Overview & Architecture

Standard delay routines (such as `delay_ms()`) block the CPU, wasting thousands of clock cycles and freezing UI, communications, and sensor monitoring. `async_delay.h` decouples time tracking from program execution:

1. **Hardware Timer Base:** A dedicated hardware timer triggers an Interrupt Service Routine (ISR) at a deterministic frequency (`ASYNC_DELAY_TICK_HZ`).
2. **Deterministic ISR Tick (`async_delay_tick`):** Increments an internal wrap-safe tick counter and checks active timers using an $O(1)$ earliest-deadline gate. Idle and non-due ticks exit in minimal cycles (~12–15 cycles) without servicing all slots.
3. **Flexible Dispatch:**
   - **ISR Callbacks:** Execute immediately inside the timer interrupt for low-jitter pin toggling or state flags.
   - **Polling Mode:** The main loop checks `async_delay_elapsed(slot_id)` and executes tasks without ISR overhead.
   - **Deferred Callbacks:** The ISR flags expired slots; `async_delay_poll()` dispatches the callbacks from the main loop context, allowing heavy operations (e.g., LCD, UART, math) with structured callback syntax.

```
       [ Hardware Timer CTC Match ]
                    │
                    ▼
          async_delay_tick()
                    │
       ┌────────────┴────────────┐
       ▼                         ▼
One-Shot / Periodic       Deferred Mode
       │                         │
  ┌────┴────┐                    ▼
  ▼         ▼           Sets Pending Mask
Callback  Polling                │
 (ISR)   (Sets EXPIRED)          ▼
            │            async_delay_poll()
            ▼                (Main Loop)
   async_delay_elapsed()         │
        (Main Loop)              ▼
                          User Callback
                           (Main Loop)
```

---

## 2. Integration & Header Precedence

`async_delay.h` relies on `SREG` and AVR register symbols provided by the chip header, and all configuration macros must precede the library include.

### Strict Include Order

```c
// 1. Target MCU header MUST come first (defines SREG, DDRx, PORTx)
#include <mega8.h>

// 2. Configuration macros defined BEFORE the library header
#define ASYNC_DELAY_TICK_HZ     1000    // Mandatory: Tick rate in Hz (1000 = 1ms)
#define ASYNC_DELAY_TIMER_BITS  16      // Optional: 8, 16 (default), or 32
#define ASYNC_DELAY_MAX_SLOTS   4       // Optional: Max concurrent timers (default 4, up to 16)

// 3. Include the library
#include <async_delay.h>
```

> **Compilation Hazard:** If `<mega8.h>` (or `<mega16.h>`, `<mega32.h>`) is omitted or included *after* `async_delay.h`, compilation fails with `undefined symbol 'SREG'`.

---

## 3. Configuration Reference

All settings are configured via preprocessor `#define` directives prior to `#include <async_delay.h>`.

### Primary Parameters

| Macro | Default | Valid Values | Description |
|---|---|---|---|
| `ASYNC_DELAY_TICK_HZ` | *None* | $> 0$ | **Required.** Interrupt rate in Hz. Example: `1000` for a 1 ms tick. |
| `ASYNC_DELAY_TIMER_BITS` | `16` | `8`, `16`, `32` | Width of the tick counter (`async_tick_t`). Determines maximum reliable delay duration. |
| `ASYNC_DELAY_MAX_SLOTS` | `4` | `1` to `16` | Maximum concurrent timers. Defaults to 4. Supports up to 16 slots with native bitmask optimization via `async_mask_t`. |

### Behavioral & Optimization Flags

| Flag | Default | Category | Architectural Impact |
|---|---|---|---|
| `ASYNC_DELAY_CALLBACK_RESCHEDULE` | `1` | Correctness | Marks a slot inactive *before* its callback runs. Enables a callback to reschedule itself via `async_delay_start()`. **Keep at `1`**. |
| `ASYNC_DELAY_OPT_FAST_CANCEL` | `1` | Performance | Bypasses $O(N)$ recompute during `async_delay_cancel()` if cancelled slot wasn't the earliest deadline. Reduces cancellation to $O(1)$ ~22 cycles. |
| `ASYNC_DELAY_FEATURE_RESTART` | `1` | Feature | Enables `async_delay_restart(slot_id, new_dur)` to retarget an existing slot in-place without freeing or changing its ID. |
| `ASYNC_DELAY_DEFERRED_CALLBACKS` | `0` | Mode | When `1`, callbacks are deferred to `async_delay_poll()` in the main loop. Allows long operations inside callbacks. |
| `ASYNC_DELAY_FIX_USED_MASK` | `1` | Correctness | Tracks allocated slots separately from running slots. Prevents expired, unpolled slots from being stolen. |
| `ASYNC_DELAY_FIX_ATOMIC_MASK` | `1` | Concurrency | Protects main-context mask updates with an SREG-preserving critical section (`#asm("cli")`). |
| `ASYNC_DELAY_OPT_BITMASK` | `1` | Performance | Restricts tick inspections to active slots via bitmask (supports up to 16 slots). |
| `ASYNC_DELAY_OPT_NEXT_TARGET` | `1` | Performance | Caches earliest target; enables $O(1)$ early exit in `async_delay_tick()` when no slots are due. |
| `ASYNC_DELAY_OPT_SPLIT_TICK` | `1` | Performance | Keeps `async_delay_tick()` free of local variables, avoiding CodeVisionAVR's `__SAVELOCR` register spill on idle ticks. |
| `ASYNC_DELAY_OPT_UNROLL_TICK` | `1` | Performance | Expands slot checks into constant indices (up to 16 slots), eliminating runtime multiplications and bit-shifts. |
| `ASYNC_DELAY_OPT_MERGED_FLAGS` | `1` | Footprint | Packs slot state and periodic flags into a single byte, saving RAM. |
| `ASYNC_DELAY_OPT_SPLIT_ARRAYS` | `0` | Footprint | Uses parallel arrays instead of an array of structures. |

### Memory Consumption & Performance

Measured on ATmega8 @ 8 MHz (`TIMER_BITS=16`, `MAX_SLOTS=4`, `TICK_HZ=1000`):

| Configuration | SRAM | Flash | ISR Idle Overhead | Main Loop Poll Overhead | CPU Load @ 1 kHz |
|---|---|---|---|---|---|
| **Default Settings** | **34 bytes** | **378 words** (~756 B) | **~85 cycles** (~10.6 µs) | — | **~1.06 %** |
| **Deferred Callbacks** (`DEFERRED=1`) | **35 bytes** (+1 B) | **397 words** (~794 B) | **~85 cycles** (~10.6 µs) | **~25 cycles** (when idle) | **~1.3 %** |

*Note: In Deferred Mode, callbacks run in main context (`async_delay_poll()`), eliminating ISR latency constraints and allowing LCD, UART, and delay operations inside callbacks.*

---

## 4. Hardware Timer Configuration (CTC Mode)

`async_delay.h` requires a timer interrupt firing at exactly `ASYNC_DELAY_TICK_HZ`. Clear Timer on Compare Match (**CTC**) mode is recommended.

### Mathematical Formula

$$\text{Timer Frequency} = \frac{F_{\text{CPU}}}{\text{Prescaler}}$$

$$\text{OCR} = \frac{\text{Timer Frequency}}{\text{ASYNC\_DELAY\_TICK\_HZ}} - 1 = \left( \frac{F_{\text{CPU}}}{\text{Prescaler} \times \text{ASYNC\_DELAY\_TICK\_HZ}} \right) - 1$$

### Standard 1 ms Tick Matrix (`TICK_HZ = 1000`)

| MCU Clock ($F_{\text{CPU}}$) | Timer Module | Prescaler | Timer Clock | Decimal OCR | CodeWizard / Hex OCR |
|---|---|---|---|---|---|
| **1.000000 MHz** | Timer2 / Timer0* | `/8` | 125.000 kHz | **124** | `0x7C` |
| **8.000000 MHz** | Timer2 | `/64` | 125.000 kHz | **124** | `0x7C` |
| **16.000000 MHz** | Timer2 | `/64` | 250.000 kHz | **249** | `0xF9` |
| **8.000000 MHz** | Timer1 (16-bit) | `/64` | 125.000 kHz | **124** | `0x007C` |
| **16.000000 MHz** | Timer1 (16-bit) | `/64` | 250.000 kHz | **249** | `0x00F9` |

*\*Note: Timer0 on ATmega8 lacks CTC mode. Use Timer2 or Timer1 on ATmega8. On ATmega16/32, Timer0 supports CTC mode.*

### CodeWizardAVR Input Warning
> **Crucial Tooling Quirk:** The "Compare" text box in CodeWizardAVR only accepts **two decimal digits**. Typing `124` gets truncated or corrupted into `80`. **Always enter values greater than 99 as hexadecimal** (e.g., write `0x7C` for 124, `0xF9` for 249).

### Ready Hardware Initialization Snippets

#### 1. ATmega8 — Timer2 CTC (8 MHz, 1 ms Tick):
```c
ASSR  = 0x00;
TCCR2 = (1 << WGM21) | (1 << CS22);  // CTC mode, prescaler 64 (CS22=1, CS21=0, CS20=0)
TCNT2 = 0x00;
OCR2  = 0x7C;                        // 124 decimal (8 MHz / 64 / 1000 - 1)
TIMSK |= (1 << OCIE2);               // Enable Timer2 compare interrupt
```

#### 2. ATmega8 / 16 / 32 — Timer1 CTC 16-bit (8 MHz or 16 MHz, 1 ms Tick):
```c
TCCR1A = 0x00;
TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode (Mode 4), prescaler 64
TCNT1H = 0x00; TCNT1L = 0x00;
// For 8 MHz: OCR1A = 124 (0x007C); For 16 MHz: OCR1A = 249 (0x00F9)
OCR1AH = 0x00; OCR1AL = 0x7C; 
TIMSK |= (1 << OCIE1A);              // Enable Timer1 Compare A interrupt
```

#### 3. ATmega16 / ATmega32 — Timer0 CTC (8 MHz, 1 ms Tick):
```c
TCCR0 = (1 << WGM01) | (1 << CS01) | (1 << CS00);  // CTC mode, prescaler 64
TCNT0 = 0x00;
OCR0  = 0x7C;                                      // 124 decimal
TIMSK |= (1 << OCIE0);                             // Enable Timer0 compare interrupt
```

---

## 5. API Reference

All library functions are declared `static` to allow clean, self-contained single-translation-unit inclusion.

### Function Summary

| Function | Signature | Execution Context | Description |
|---|---|---|---|
| `async_delay_init` | `void async_delay_init(void)` | Main (Init only) | Clears all slots, resets masks, zeroes counter. Must be called once before `sei`. |
| `async_delay_start` | `unsigned char async_delay_start(async_tick_t duration, async_delay_cb_t cb)` | Main / Callback | Starts a one-shot delay. Returns slot ID ($0$ to $\text{MAX}-1$) or `0xFF` on failure. |
| `async_delay_start_periodic` | `unsigned char async_delay_start_periodic(async_tick_t duration, async_delay_cb_t cb)` | Main / Callback | Starts an auto-rearming periodic timer. Returns slot ID or `0xFF`. |
| `async_delay_elapsed` | `unsigned char async_delay_elapsed(unsigned char slot_id)` | Main Loop | Polls a one-shot timer. Returns `1` and frees the slot if expired, else `0`. |
| `async_delay_restart` | `unsigned char async_delay_restart(unsigned char slot_id, async_tick_t new_dur)` | Main / Callback | Retargets an allocated timer in-place with a new duration, keeping the same slot ID. Returns `1` on success, `0` on error. |
| `async_delay_cancel` | `void async_delay_cancel(unsigned char slot_id)` | Main / Callback | Cancels a running or expired timer and reclaims its slot. |
| `async_delay_tick` | `void async_delay_tick(void)` | Hardware ISR Only | Increments the internal tick counter and evaluates due timers. |
| `async_delay_poll` | `void async_delay_poll(void)` | Main Loop Only | Dispatches pending deferred callbacks (only when `DEFERRED_CALLBACKS=1`). |

### Callback Type
```c
typedef void (*async_delay_cb_t)(unsigned char slot_id);
```
- Passing `(void *)0` (or `NULL`) as the callback configures the slot for **Polling Mode**.
- Passing a function pointer configures **Callback Mode**.

### Slot State Transitions

```
                    ┌─────────────────────────┐
                    │          FREE           │◄─────────────────────┐
                    └────────────┬────────────┘                      │
                                 │ async_delay_start()               │
                                 ▼                                   │
                    ┌─────────────────────────┐                      │
                    │         ACTIVE          │                      │
                    └──────┬───────────┬──────┘                      │
                           │           │                             │
    [One-Shot + Callback]  │           │ [One-Shot + Polling]        │
   async_delay_tick fires  │           │ async_delay_tick fires      │
                           │           ▼                             │
                           │      ┌───────────┐                      │
                           │      │  EXPIRED  │                      │
                           │      └─────┬─────┘                      │
                           │            │ async_delay_elapsed() == 1 │
                           │            └────────────────────────────┤
                           ▼                                         │
                    (Slot Released)                                  │
                           │                                         │
                           └─────────────────────────────────────────┘
                                   async_delay_cancel()
```

---

## 6. Critical Invariants & Rules for AI Agents

When writing firmware using `async_delay.h`, adhere strictly to these engineering constraints:

### 1. The Polling Slot Allocation Leak
When a timer is started without a callback (`cb = (void *)0`), the slot transitions to `ASYNC_SLOT_EXPIRED` upon reaching its deadline.
- **The slot remains allocated until `async_delay_elapsed(slot_id)` returns `1` or `async_delay_cancel(slot_id)` is called.**
- If your program starts a polling timer but stops calling `async_delay_elapsed()`, the slot is **permanently leaked**. After leaking `ASYNC_DELAY_MAX_SLOTS` times, subsequent calls to `async_delay_start()` will fail and return `0xFF` (`ASYNC_DELAY_NO_SLOT`).

### 2. Time Units & Overflow Bounds
- `duration` is in **ticks**, not milliseconds:
  $$\text{ticks} = \frac{\text{ms} \times \text{ASYNC\_DELAY\_TICK\_HZ}}{1000}$$
- `duration = 0` is valid and expires on the immediately following tick.
- Maximum reliable delay is **strictly half the counter range** due to wrap-safe unsigned arithmetic:
  - `TIMER_BITS = 8`: Max 127 ticks ($127\text{ ms}$ at 1 kHz).
  - `TIMER_BITS = 16`: Max 32,767 ticks ($\approx 32.76\text{ seconds}$ at 1 kHz).
  - `TIMER_BITS = 32`: Max 2,147,483,647 ticks ($\approx 24.85\text{ days}$ at 1 kHz).
  *Attempting to schedule a delay longer than half the range causes immediate or premature expiration.*

### 3. Slot ID Recycling
Slot IDs are integers from `0` to `ASYNC_DELAY_MAX_SLOTS - 1`. Once a timer finishes or is cancelled, its ID is returned to the pool and may be reassigned on the very next `async_delay_start()`.
- **Never call `async_delay_elapsed(id)` or `async_delay_cancel(id)` using a stale ID from a previously completed timer.** It may inadvertently clear a different, newly scheduled task.

### 4. Self-Rescheduling Contract
- **One-Shot:** Calling `async_delay_start()` from inside its own callback is fully supported and recommended for finite state sequences. The current slot is deallocated *before* the callback is entered, allowing the same slot to be reused immediately.
- **Periodic:** A periodic timer re-arms *before* its callback runs. Do **not** call `async_delay_start()` on the same slot from inside a periodic callback, as this allocates a secondary concurrent timer. To alter the period of a periodic timer from its callback, call `async_delay_cancel(slot_id)` first.

### 5. Multi-File Compilation Contract (`static` Linkage)
All library functions and internal data structures are defined with `static` linkage.
- **`async_delay.h` must be included in exactly ONE `.c` compilation unit** (e.g., `main.c` or a dedicated `timer_service.c`).
- If included in multiple `.c` files, each file instantiates its own isolated copy of variables (`_async_tick_counter`, slots, masks). The ISR will only increment the instance in its own file, leaving the others frozen.
- Share timing state across files using `extern` flags or interface functions (see Pattern 5).

### 6. CodeVisionAVR Compiler Constraints
- **C89 Variable Declarations:** All variable declarations must appear at the beginning of a code block, before any executable statements.
- **Reserved Keywords:** The word `bit` is an intrinsic CodeVisionAVR storage type specifier. **Never use `bit` as an identifier, variable, or parameter name.** Use `slot_bit` or `mask_bit`.
- **Literal Suffixes:** Avoid `u` or `U` integer literal suffixes (e.g., use `1000`, not `1000U`).
- **Inline Assembly:** Global interrupts must be enabled using `#asm("sei")` and disabled using `#asm("cli")`.

---

## 7. Verified Implementation Patterns

### Pattern 0: Minimal Robust Template (ATmega8 @ 8 MHz, 1 ms Tick)

```c
#include <mega8.h>

// 1. Library configuration
#define ASYNC_DELAY_TICK_HZ     1000
#define ASYNC_DELAY_TIMER_BITS  16
#define ASYNC_DELAY_MAX_SLOTS   4
#include <async_delay.h>

// ISR: Must ONLY call async_delay_tick()
interrupt [TIM2_COMP] void timer2_comp_isr(void)
{
    async_delay_tick();
}

static unsigned char g_led_state = 0;

// Callback: Executed inside ISR context - keep execution under a few microseconds!
void blink_callback(unsigned char slot_id)
{
    g_led_state ^= 1;
}

void main(void)
{
    // Configure PB0 as output (LED)
    DDRB.0 = 1;
    PORTB.0 = 0;

    // Timer2 Setup: CTC mode, Prescaler /64, 8MHz -> 125kHz clock
    // OCR2 = (8000000 / 64 / 1000) - 1 = 124 (0x7C in hex)
    TCCR2 = (1 << WGM21) | (1 << CS22);  // CTC mode, prescaler 64
    TCNT2 = 0x00;
    OCR2  = 0x7C;                        // 124 decimal
    TIMSK |= (1 << OCIE2);               // Enable Timer2 Compare Match interrupt
    ASSR  = 0x00;

    // Initialize the async delay engine
    async_delay_init();

    // Start a 500 ms periodic blink timer
    async_delay_start_periodic(500, blink_callback);

    // Enable global interrupts after all hardware & library inits are complete
    #asm("sei")

    while (1)
    {
        PORTB.0 = g_led_state;
        // Main loop remains 100% free for user tasks
    }
}
```

---

### Pattern 1: Polling Mode for Heavy Operations (LCD / Sensor)

Never update an LCD, read an I2C/1-Wire sensor, or run `printf` inside an ISR callback. Use Polling Mode instead:

```c
void main(void)
{
    unsigned char lcd_timer_id;

    // ... hardware and timer initializations ...
    async_delay_init();
    #asm("sei")

    // Schedule 200 ms non-blocking polling timer (callback is NULL)
    lcd_timer_id = async_delay_start(200, (void *)0);

    while (1)
    {
        // Check if 200 ms have elapsed
        if (async_delay_elapsed(lcd_timer_id))
        {
            // Slot was automatically released by async_delay_elapsed()
            update_lcd_display();

            // Re-arm the polling timer
            lcd_timer_id = async_delay_start(200, (void *)0);
        }

        // Other non-blocking background logic
    }
}
```

---

### Pattern 2: Self-Rescheduling Finite State Machine

Step through multi-stage sequences without delays or nested switches:

```c
static unsigned char g_seq_step = 0;

void sequence_callback(unsigned char slot_id)
{
    g_seq_step++;

    switch (g_seq_step)
    {
        case 1:
            PORTB.1 = 1; // Turn relay ON
            async_delay_start(100, sequence_callback); // Hold for 100ms
            break;

        case 2:
            PORTB.1 = 0; // Turn relay OFF
            async_delay_start(500, sequence_callback); // Rest for 500ms
            break;

        case 3:
            PORTB.2 = 1; // Signal complete
            // Sequence terminates: no new start() call
            break;
    }
}

// Trigger sequence:
void trigger_sequence(void)
{
    g_seq_step = 0;
    async_delay_start(10, sequence_callback);
}
```

---

### Pattern 3: Deferred Callbacks for Heavy Work (`DEFERRED_CALLBACKS=1`)

If you prefer callback architecture but must perform substantial processing (e.g. UART transmissions), enable deferred mode:

```c
#include <mega8.h>

#define ASYNC_DELAY_TICK_HZ           1000
#define ASYNC_DELAY_DEFERRED_CALLBACKS 1   // Enable deferred dispatch
#include <async_delay.h>

interrupt [TIM2_COMP] void timer2_comp_isr(void)
{
    async_delay_tick(); // Only records pending status in a bitmask
}

// Executed in MAIN LOOP context via async_delay_poll()
void uart_report_callback(unsigned char slot_id)
{
    // Heavy operations are completely safe here!
    putchar('T');
    putchar('I');
    putchar('C');
    putchar('K');
    putchar('\r');
    putchar('\n');
}

void main(void)
{
    // ... timer setup ...
    async_delay_init();
    async_delay_start_periodic(1000, uart_report_callback);
    #asm("sei")

    while (1)
    {
        // Mandatory in deferred mode: dispatches due callbacks
        async_delay_poll();

        // Background application processing
    }
}
```

---

### Pattern 4: Safe Periodic Retiming & Watchdog Resets (`async_delay_restart`)

To change the interval of an active timer or reset a watchdog/debounce timeout without reallocating a new slot ID, use `async_delay_restart`:

```c
static unsigned char g_heartbeat_id = 0xFF;

void init_heartbeat(void)
{
    g_heartbeat_id = async_delay_start_periodic(1000, heartbeat_cb);
}

void set_heartbeat_rate(unsigned int ms_rate)
{
    // Restarts in-place without slot reallocation or ID changes:
    if (g_heartbeat_id != 0xFF)
    {
        async_delay_restart(g_heartbeat_id, ms_rate);
    }
    else
    {
        g_heartbeat_id = async_delay_start_periodic(ms_rate, heartbeat_cb);
    }
}

// Watchdog / debounce reset pattern:
void kick_communication_watchdog(unsigned char wdt_slot_id)
{
    // Resets the deadline to 3000ms from now:
    async_delay_restart(wdt_slot_id, 3000);
}
```

---

### Pattern 5: Modular Multi-File Architecture

Encapsulate the timer engine inside a single module to prevent symbol duplication:

#### `timer_service.h`
```c
#ifndef _TIMER_SERVICE_H_
#define _TIMER_SERVICE_H_

void timer_service_init(void);
extern volatile unsigned char g_flag_sensor_due;
extern volatile unsigned char g_flag_ui_due;

#endif
```

#### `timer_service.c`
```c
#include <mega8.h>
#include "timer_service.h"

#define ASYNC_DELAY_TICK_HZ 1000
#include <async_delay.h>

volatile unsigned char g_flag_sensor_due = 0;
volatile unsigned char g_flag_ui_due = 0;

static void sensor_tick_cb(unsigned char slot_id) { g_flag_sensor_due = 1; }
static void ui_tick_cb(unsigned char slot_id)     { g_flag_ui_due = 1; }

interrupt [TIM2_COMP] void timer2_isr(void)
{
    async_delay_tick();
}

void timer_service_init(void)
{
    // Hardware CTC setup
    TCCR2 = (1 << WGM21) | (1 << CS22);
    OCR2  = 0x7C;
    TIMSK |= (1 << OCIE2);

    async_delay_init();
    async_delay_start_periodic(50,  sensor_tick_cb); // 50ms sensor tick
    async_delay_start_periodic(200, ui_tick_cb);     // 200ms UI tick
}
```

#### `main.c`
```c
#include <mega8.h>
#include "timer_service.h"
// NOTE: DO NOT #include <async_delay.h> here!

void main(void)
{
    timer_service_init();
    #asm("sei")

    while (1)
    {
        if (g_flag_sensor_due)
        {
            g_flag_sensor_due = 0;
            read_and_process_sensors();
        }

        if (g_flag_ui_due)
        {
            g_flag_ui_due = 0;
            refresh_display();
        }
    }
}
```

---

## 8. Diagnostic Decision Tree & Troubleshooting

| Symptom | Root Cause | Exact Remedy |
|---|---|---|
| **Timers never fire / `elapsed()` always returns 0** | Global interrupts disabled or timer not interrupting. | 1. Verify `#asm("sei")` is called after all initializations.<br>2. Check `TIMSK` (e.g. `TIMSK |= (1 << OCIE2)`).<br>3. Verify timer CTC bits (`WGM21`). |
| **All delays run 2x slower or 2x faster** | Clock mismatch or OCR calculation error. | 1. Ensure project frequency in CodeVisionAVR matches hardware (e.g. 8.000000 MHz vs 16.000000 MHz).<br>2. Recalculate OCR: $\text{OCR} = (F_{\text{CPU}} / \text{Prescaler} / 1000) - 1$. |
| **`async_delay_start()` returns `0xFF` (`ASYNC_DELAY_NO_SLOT`)** | Slot exhaustion / polling slot memory leak. | 1. Confirm you are calling `async_delay_elapsed()` for all polling slots.<br>2. Increase `ASYNC_DELAY_MAX_SLOTS` up to 8.<br>3. Verify cancelled tasks call `async_delay_cancel()`. |
| **Deferred callbacks never execute** | Missing main loop dispatch. | When `ASYNC_DELAY_DEFERRED_CALLBACKS = 1`, you **must** call `async_delay_poll()` inside `while(1)`. |
| **System freezes or UART/LCD corrupts** | Lengthy code executed inside ISR callback. | Move heavy processing to the main loop using Polling Mode or Deferred Mode. |
| **Compile Error: `undefined symbol 'SREG'`** | Header inclusion order error. | `#include <mega8.h>` must precede `#include <async_delay.h>`. |
| **Compile Error: `invalid combination of type specifiers`** | Keyword collision. | Look for variables named `bit` and rename them (e.g. `slot_bit`). |
| **Compile Error: `must declare first in block`** | C89 violation. | Move all variable declarations to the top of the function/block before any code statements. |
| **Compile Error: `...at most 8 slots`** | Configuration violation. | When bitmask optimization is active, `ASYNC_DELAY_MAX_SLOTS` cannot exceed 8. |

---

## 9. Version Synchronization Contract

`async_delay.h` contains a build synchronization constant:
```c
#define ASYNC_DELAY_VERSION 7
```

To guard against silent regressions or outdated local copies across external project directories, add this compile-time assertion to your application headers:

```c
#if ASYNC_DELAY_VERSION != 7
#error "async_delay.h version mismatch! Update the header copy in your include path."
#endif
```
*(This validation executes strictly in the preprocessor and incurs zero Flash or SRAM overhead).*
