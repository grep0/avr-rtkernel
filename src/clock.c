#include "clock.h"

#include <avr/builtins.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

// Clock increments every 1ms.
// 32 bit is enough for 49.7 days, hoefully we won't overflow
static volatile uint32_t clock_ms = 0;

ISR(TIMER0_COMPA_vect) {
	++clock_ms;
}

uint32_t get_clock_ms() {
	// Get atomically to avoid interrupt in the middle of reading
	cli();
	uint32_t c = clock_ms;
	sei();
	return c;
}

void setup_timer() {
	// Set TIMER_COUNTER_0 to toggle interrupt every 1ms
	TCCR0A = 2;  // No OC0A, no OC0B, CTC
	TCCR0B = 3;  // CLK/64
	OCR0A = 249; // 16M/64/(1+249) = 1000 Hz
	TIMSK0 = 2;  // Allow interrupts on OCR0A

}
