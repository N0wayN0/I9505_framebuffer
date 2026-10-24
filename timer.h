

//typedef struct timer_t {
struct timer_t {
    long fire_at_ms;    /* absolute deadline; -1 = slot free */
    long interval_ms;  /* 0 = one-shot, >0 = repeat */
    void (*handler)(int id);
    int  id;            /* passed to handler so one func can serve many */
};

//#define MAX_TIMERS 8
//static timer_t timers[MAX_TIMERS];

static long now_ms(void);
int timer_add(long delay_ms, long repeat_ms, void (*handler)(int), int id);
void timer_cancel(int slot);
int timer_next_ms(void);
void timer_check(void);
void init_timers(void);
void print_timer(int slot);
