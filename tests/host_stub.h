// host_stub.h - stand-ins for CodeVisionAVR-only constructs, HOST TESTS ONLY.
// Never included by firmware. The firmware header (async_delay.h) is
// self-contained except for ONE MCU-provided symbol: SREG (from mega8.h),
// which this stub provides so the real header compiles under gcc on the host.
//
// Why this is enough:
//  * #asm("cli"/"sei") is stripped by make_host.py (search/replace the #-prefix),
//    so it never reaches gcc. On the single-threaded host ISR disabling is
//    meaningless anyway - see plans/006 plan §4.7 on what host tests cannot prove.
//  * `bit`, `flash`, `eeprom`, `sfrb/sfrw`, `interrupt`, `funcused` appear in
//    the header ONLY inside comments - no stubs needed.
//  * mega8.h symbols (DDR, PORT, TCNT, etc.) are in test driver code only;
//    this stub is never included where they appear.
#ifndef _ASYNC_DELAY_HOST_STUB_
#define _ASYNC_DELAY_HOST_STUB_

// SREG is the only MCU register the header reads/writes (via the
// _ASYNC_SAVE_SREG / _ASYNC_REST_SREG macros when ASYNC_DELAY_FIX_ATOMIC_MASK=1).
// A plain byte is the right model: 8-bit AVR SREG is 8 bits, and on the host
// there are no real interrupts to disable. Declared `static` so multiple test
// translation units that each #include the (generated) header plus this file
// do not collide at link time. `unused` keeps FIX_ATOMIC_MASK=0 combos (where
// the header never touches SREG) warning-free under -Werror.
static unsigned char _async_delay_host_SREG __attribute__((unused));
#define SREG _async_delay_host_SREG

#endif
