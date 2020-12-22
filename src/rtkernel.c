#include "rtkernel.h"

#include <avr/io.h>
#include <setjmp.h>
#include <assert.h>
#include <avr/sleep.h>

#include "clock.h"

jmp_buf jmp_kernel;

enum {
    T_USED = 0x1,
    T_ACTIVE = 0x2,
    T_SLEEPING = 0x4,
};

struct jmpbuf_internals {
    uint8_t call_saved_regs[18];  // r2-r17,r28,r29
    uint16_t sp;  // SPH:SPL
    uint8_t sreg;
    uint16_t return_addr;
} __attribute__((packed));

jmp_buf thread_jmp[NUM_THREADS];
uint8_t thread_state[NUM_THREADS];
uint32_t thread_sleep_deadline[NUM_THREADS];
uint8_t thread_stack[NUM_THREADS][THREAD_STACK_SIZE];

thread_id current_tid = 0xFF;

enum {
    KJ_YIELD = 1,
    KJ_FREEZE = 2,
    KJ_CONTINUE = 3,
    KJ_YIELD_TO = 0x10,  // YIELD_TO+thread_id
};

static bool thread_is_ready(thread_id tid, uint32_t clk) {
    uint8_t t_flags =  thread_state[tid];
    if (!(t_flags & T_USED)) {
        return false;
    }
    if (t_flags & T_SLEEPING) {
        // Rollover-safe way to compare
        if (!((thread_sleep_deadline[tid]-clk)&0x80000000UL)) {
            thread_state[tid] &= ~T_SLEEPING;
            return true;
        }
        return false;
    }
    return true;
}

#define STACK_SENTINEL 0x5A
static void verify_stack(thread_id tid) {
    struct jmpbuf_internals* jbuf = (struct jmpbuf_internals*)(thread_jmp[tid]);
    // SP is within the limits
    assert(jbuf->sp >= (uint16_t)thread_stack[tid] && jbuf->sp < (uint16_t)thread_stack[tid] + THREAD_STACK_SIZE);
    // and sentinels are there
    assert(thread_stack[tid][0] == STACK_SENTINEL && thread_stack[tid][THREAD_STACK_SIZE-1] == STACK_SENTINEL);
}

void start_kernel() {
    assert(sizeof(struct jmpbuf_internals) == sizeof(jmp_buf));
    assert(current_tid == 0xFF);
    int kj = setjmp(jmp_kernel);
    if (kj != 0) {
        if (current_tid < NUM_THREADS) {
            verify_stack(current_tid);
        }
        if (kj == KJ_FREEZE) {
            // freeze current task
            if (current_tid < NUM_THREADS) {
                thread_state[current_tid] = 0;
            }
        } else {
            assert(current_tid < NUM_THREADS);
            thread_state[current_tid] &= ~T_ACTIVE;
        }
        for(;;) {
            uint32_t current_clock = get_clock_ms();
            // Find next thread to spawn
            bool next_tid_found = false;
            thread_id next_tid = 0xFF;
            if (kj >= KJ_YIELD_TO) {
                next_tid = kj - KJ_YIELD_TO;
                if (next_tid < NUM_THREADS && next_tid != current_tid) {
                    if (thread_is_ready(next_tid, current_clock)) {
                        next_tid_found = true;
                    }
                }
            }
            if (!next_tid_found) {
                if (current_tid >= NUM_THREADS) {
                    next_tid = 0;
                } else {
                    next_tid = (current_tid+1) % NUM_THREADS;
                }
                for(uint8_t i=0; i<NUM_THREADS; ++i) {
                    if (thread_is_ready(next_tid, current_clock)) {
                        next_tid_found = true;
                        break;
                    }
                    next_tid = (next_tid+1)%NUM_THREADS;
                }
            }
            if (next_tid_found) {
                current_tid = next_tid;
                thread_state[current_tid] |= T_ACTIVE;
                longjmp(thread_jmp[current_tid], KJ_CONTINUE);
            }
            sleep_cpu();
        }
    }
    // kj == 0
}

void freeze() {
    longjmp(jmp_kernel, KJ_FREEZE);
}

void yield() {
    assert(current_tid < NUM_THREADS);
    if (setjmp(thread_jmp[current_tid]) == 0) {
        longjmp(jmp_kernel, KJ_YIELD);
    }
}

void yield_to(thread_id tid) {
    assert(current_tid < NUM_THREADS);
    if (setjmp(thread_jmp[current_tid]) == 0) {
        longjmp(jmp_kernel, KJ_YIELD_TO + tid);
    }
}

void sleep_until(uint32_t ms) {
    thread_state[current_tid] |= T_SLEEPING;
    thread_sleep_deadline[current_tid] = ms;
    yield();
}

void sleep_ms(uint16_t ms) {
    sleep_until(get_clock_ms() + ms);
}

void sleep_ms_long(uint32_t ms) {
    sleep_until(get_clock_ms() + ms);
}

void spawn_trampoline() __attribute__((naked));

thread_id spawn(entry_point_t fn, void* arg) {
    thread_id new_tid = 0;
    for (; new_tid < NUM_THREADS; ++new_tid) {
        if (!(thread_state[new_tid] & T_USED)) break;
    }
    if (new_tid >= NUM_THREADS) {
        // oops, no new threads available
        return 0xFF;
    }
    // Set up jmpbuf to start
    struct jmpbuf_internals* jbuf = (struct jmpbuf_internals*)(thread_jmp[new_tid]);
    // Set stack sentinel
    thread_stack[new_tid][0] = STACK_SENTINEL;
    thread_stack[new_tid][THREAD_STACK_SIZE-1] = STACK_SENTINEL;
    jbuf->sreg = SREG;
    jbuf->sp = (uint16_t)(&thread_stack[new_tid][THREAD_STACK_SIZE-2]);
    jbuf->return_addr = (uint16_t)(void*)(&spawn_trampoline);
    jbuf->call_saved_regs[16] = ((uint16_t)fn & 0xFF);  // r28
    jbuf->call_saved_regs[17] = ((uint16_t)fn >> 8);    // r29
    jbuf->call_saved_regs[14] = ((uint16_t)arg & 0xFF); // r16
    jbuf->call_saved_regs[15] = ((uint16_t)arg >> 8);   // r17
    thread_state[new_tid] = T_USED;

    return new_tid;
}

void spawn_trampoline() {
    asm volatile(
        "movw r24, r16\n\t"  // arg
        "movw r30, r28\n\t"  // fn
        "icall\n\t"
        ::: "memory"
    );
    freeze();
}