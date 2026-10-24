#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#define MAX_TOUCH_AREAS  64
//int areas_count = 0;

struct touch_area {
    int  id;
    int  x, y, w, h;
    int  enabled;
    char action[MAX_TOUCH_AREAS];
};

//struct touch_area areas[MAX_TOUCH_AREAS];

// Manage areas
int  touch_add_area(const struct touch_area *area);
int  touch_remove_area(int id);
int  touch_update_areas(const struct touch_area *areas, int count);
void touch_clear_areas(void);

// Device / event handling
int  input_init(void);
int  input_get_fds(int max);
//int  input_get_fds(struct pollfd *fds, int max);
//int  input_poll(struct touch_area *out, struct pollfd fds, int nfds);
int  input_poll(struct touch_area *out, int nfds, int timeout);
void input_close(void);

#endif
