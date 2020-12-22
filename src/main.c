#include "clock.h"
#include "rtkernel.h"
#include <avr/io.h>
#include <avr/interrupt.h>

void worker1(void* arg) {
    uint32_t now = get_clock_ms();
    for(;;) {
        PORTB |= (1<<5);
        yield_until(now+=200);
        PORTB &= ~(1<<5);
        yield_until(now+=800);
    }
}

void worker3(void* arg) {
    uint32_t now = get_clock_ms();
    yield_until(now+=50);
    PORTB |= (1<<0);
    yield_until(now+=50);
    PORTB &= ~(1<<0);
}

void worker2(void* arg) {
    uint32_t now = get_clock_ms();
    for(;;) {
        PORTB |= (1<<2);
        yield_until(now+=300);
        PORTB &= ~(1<<2);
        yield_until(now+=600);
        spawn(worker3, 0);
    }
}

int main() {
    DDRB=0xFF;
    SMCR = 0x1;  // sleep enabled, mode=idle
    sei();
    setup_timer();
    start_kernel();
    spawn(worker1, 0);
    spawn(worker2, 0);
    freeze();
}