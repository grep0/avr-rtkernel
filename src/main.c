#include "clock.h"
#include "rtkernel.h"
#include <avr/io.h>
#include <avr/interrupt.h>

void worker1(void* arg) {
    uint32_t now = get_clock_ms();
    for(;;) {
        PORTB |= (1<<5);
        sleep_until(now+=200);
        PORTB &= ~(1<<5);
        sleep_until(now+=800);
    }
}

void worker3(void* arg);

struct worker3_arg {
    uint16_t pause_ms;
    uint16_t signal_ms;
};

void worker2(void* arg) {
    uint32_t now = get_clock_ms();
    struct worker3_arg warg = { 50, 50 };
    for(;;) {
        PORTB |= (1<<2);
        sleep_until(now+=300);
        PORTB &= ~(1<<2);
        sleep_until(now+=600);
        spawn(worker3, &warg);
    }
}

void worker3(void* arg) {
    struct worker3_arg arg_copy = *(struct worker3_arg*)arg;
    sleep_ms(arg_copy.pause_ms);
    PORTB |= (1<<0);
    sleep_ms(arg_copy.signal_ms);
    PORTB &= ~(1<<0);
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