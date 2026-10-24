#include "timer.h"
#include <stdio.h>
#include <time.h>


#define MAX_TIMERS 8
static struct timer_t timers[MAX_TIMERS];

static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// delay_ms: first fire. repeat_ms: 0 = one-shot, >0 = repeat.
int timer_add(long delay_ms, long repeat_ms, void (*handler)(int), int id) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        struct timer_t *timer = &timers[i];
        if (timer->fire_at_ms < 0) {
            timer->fire_at_ms = now_ms() + delay_ms;
            timer->interval_ms = repeat_ms;
            timer->handler     = handler;
            timer->id          = id;
            return i;              // slot index, or use as handle
        }
    }
    return -1;                     // full
}

void timer_cancel(int slot) {
    if (slot >= 0 && slot < MAX_TIMERS)
        timers[slot].fire_at_ms = -1;
}

//Returns the nearest deadline, clamped to 0. If no timers are active,
int timer_next_ms(void)
{
    long now = now_ms();
    long best = -1;               // -1 = no timers active

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].fire_at_ms < 0) continue;
        long d = timers[i].fire_at_ms - now;
        if (d < 0) d = 0;         // already due
        if (best < 0 || d < best) best = d;
    }
    return (int)best;
}

void timer_check(void)
{
    long now = now_ms();

    for (int i = 0; i < MAX_TIMERS; i++) {
        struct timer_t *t = &timers[i];
        if (t->fire_at_ms < 0) continue;
        if (t->fire_at_ms > now) continue;

        if (t->interval_ms > 0) {
            t->fire_at_ms += t->interval_ms;
            // if we fell behind, resync instead of firing N times
            if (t->fire_at_ms < now)
                t->fire_at_ms = now + t->interval_ms;
        } else {
            t->fire_at_ms = -1;   // one-shot: free the slot
        }

        if (t->handler)
            t->handler(t->id);
    }
}

void init_timers(void)
{
    for (int i = 0; i < MAX_TIMERS; i++)
        timers[i].fire_at_ms = -1;      // mark all free
}

void print_timer(int slot) {
    struct timer_t *t = &timers[slot];
    printf("(%s) Print Timer %d\n", __func__, slot);
    if (t == NULL) {
        printf("timer_t: (null)\n");
        return;
    }

    printf("  id          = %d\n", t->id);

    if (t->fire_at_ms == -1)
        printf("  fire_at_ms  = -1 (slot free)\n");
    else
        printf("  fire_at_ms  = %ld\n", t->fire_at_ms);

    if (t->interval_ms == 0)
        printf("  interval_ms = 0 (one-shot)\n");
    else
        printf("  interval_ms = %ld (repeating)\n", t->interval_ms);

    if (t->handler == NULL)
        printf("  handler     = (null)\n");
    else
        printf("  handler     = %p\n", (void *)t->handler);

}
