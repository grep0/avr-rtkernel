#ifndef _rtkernel_h
#define _rtkernel_h

#include <stdint.h>
#include <stdbool.h>

#define NUM_THREADS 4
#define THREAD_STACK_SIZE 64
typedef uint8_t thread_id;
extern thread_id current_thread_id;

void start_kernel();

typedef void(*entry_point_t)(void*);
thread_id spawn(entry_point_t fn, void* arg);

// Finish current thread. You can just return instead
void freeze() __attribute__((noreturn));
// Yield and let other threads go.
void yield();
// Yield, run the specified thread next if it's ready.
void yield_to(thread_id id);
// Sleep until given time (ms since program start)
void yield_until(uint32_t ms);

#endif