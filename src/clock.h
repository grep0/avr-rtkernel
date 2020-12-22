#ifndef _clock_h
#define _clock_h

#include <stdint.h>

// Crystal frequency is 16MHZ unless defined otherwise
#ifndef F_CPU
#define F_CPU 16000000
#endif
#include <util/delay.h>

// Setup timer0 at 1KHz
void setup_timer();
// Get millisecond clock since startup (overflows in ~49.7 days)
uint32_t get_clock_ms();

#endif
