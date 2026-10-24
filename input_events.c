#include "input_events.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/msm_mdp.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/wait.h>

#define VOLUME_HOME "/dev/input/event18"
#define TOUCH_BUTTONS "/dev/input/event19"
#define POWER_BUTTON "/dev/input/event0"
#define TOUCHSCREEN "/dev/input/event3"

#define KEY_VOLDOWN	(114)
#define KEY_VOLUP 	(115)
#define KEY_HOME	(172)
#define KEY_POWER	(116)
#define KEY_BACK	(158)
#define KEY_MENU	(139)

// input events
int fd_power_home, fd_tbtn, fd_power, fd_tscreen;
struct pollfd fds[8];
extern ret;
int touch_x = -1, touch_y = -1;
int touch_active = 0;
int areas_count = 0;
static int touch_in_area = -1;  /* index of area being touched, -1 if none */
struct touch_area areas[MAX_TOUCH_AREAS];
int left_menu = 0;
int right_menu = 0;


// Open all input devices
int input_init() {
    fd_power_home = open(VOLUME_HOME, O_RDONLY | O_NONBLOCK);
    fd_tbtn = open(TOUCH_BUTTONS, O_RDONLY | O_NONBLOCK);
    fd_power = open(POWER_BUTTON, O_RDONLY | O_NONBLOCK);
    fd_tscreen = open(TOUCHSCREEN, O_RDONLY | O_NONBLOCK);

    printf("(%s) Vol and Home fd:   %d\n", __func__, fd_power_home);
    printf("(%s) Touch buttons fd:  %d\n", __func__, fd_tbtn);
    printf("(%s) Power button fd:   %d\n", __func__, fd_power);
    printf("(%s) Touchscreen fd:    %d\n", __func__, fd_tscreen);
}

// Set up the pollfd array
//int input_get_fds(struct pollfd *fds, int max) {
int input_get_fds(int max) {
    int n = 0;
    if (fd_power_home >=0 && n < max) {
        fds[n].fd = fd_power_home;
        fds[n].events = POLLIN;
        n++;
    }

    if (fd_tbtn >=0 && n < max) {
        fds[n].fd = fd_tbtn;
        fds[n].events = POLLIN;
        n++;
    }

    if (fd_power >=0 && n < max) {
        fds[n].fd = fd_power;
        fds[n].events = POLLIN;
        n++;
    }

    if (fd_tscreen >=0 && n < max) {
        fds[n].fd = fd_tscreen;
        fds[n].events = POLLIN;
        n++;
    }
    //printf("(%s) FDS:    %d\n", __func__, n);
    return n;
}

int input_poll(struct touch_area *out, int n, int timeout) {
    int ret = poll(fds, n, 50);
    if (ret < 0) {
        perror("poll failed");
        return -1;
    }
    if (ret == 0) {
        return 0;  /* timeout */
    }

    for (int i = 0; i < n; i++) {
        if (!(fds[i].revents & POLLIN)) continue;

        struct input_event ev;
        ssize_t r;
        while ((r = read(fds[i].fd, &ev, sizeof(ev))) == sizeof(ev)) {
            /* --- accumulate touch state --- */
            if (ev.type == EV_ABS) {
                if (ev.code == ABS_X || ev.code == ABS_MT_POSITION_X)
                    touch_x = ev.value;
                else if (ev.code == ABS_Y || ev.code == ABS_MT_POSITION_Y)
                    touch_y = ev.value;
            } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                touch_active = ev.value;
            } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                /* --- frame complete: act only on state transitions --- */
                if (touch_active && touch_in_area < 0) {
                    /* finger just went down, find the area */
                    for (int k = 0; k < areas_count; k++) {
                        struct touch_area *a = &areas[k];
                        if (!a->enabled) continue;
                        if (touch_x >= a->x && touch_x < a->x + a->w &&
                            touch_y >= a->y && touch_y < a->y + a->h) {
                            touch_in_area = k;
                            if (out) *out = *a;
                            return 1;   /* one hit per physical press */
                        }
                    }
                } else if (!touch_active) {
                    /* finger lifted allow the next press to register */
                    touch_in_area = -1;
                }
                /* else: touch_active && touch_in_area >= 0
                   finger is still down inside the area, ignore */
            }

            /* --- key handling --- */
            if (ev.type == EV_KEY) {
                if (ev.code == KEY_VOLUP) {
                    printf("Volume Up %s\n", ev.value ? "Pressed" : "Released");
                    if (ev.value == 1) {
                        printf("vol up key\n");
                    } else {
                        printf("vol up key\n");
                    }
                } else if (ev.code == KEY_VOLDOWN) {
                    printf("Volume Down %s\n", ev.value ? "Pressed" : "Released");
                    if (ev.value == 1) {
                        printf("vol dn key\n");
                    } else {
                        printf("vol dn key\n");
                    }
                } else if (ev.code == KEY_POWER) {
                    printf("Power Key %s\n", ev.value ? "Pressed" : "Released");
                    if (ev.value == 1) {
                        printf("power dn key\n");
                    } else {
                        printf("power dn key\n");
                    }
                } else if (ev.code == KEY_MENU) {
                    printf("Menu Key %s\n", ev.value ? "Pressed" : "Released");
                    if (ev.value == 1) {
                        printf("menu key\n");
                    }
                } else if (ev.code == KEY_BACK) {
                    printf("Back key %s\n", ev.value ? "pressed" : "released");
                    if (ev.value == 1) {
                        printf("back key\n");
                    } else {
                        printf("back key\n");
                    }
                } else if (ev.code == 172 ) { //KEY_HOME) {
                    printf("Home key %s\n", ev.value ? "pressed" : "released");
                    if (ev.value == 1) {
                        printf("back key\n");
                    } 
                }
            } // if (ev.type == EV_KEY)
        } // while
    } //for
    return 0;
}




void input_close(void)
{
    if (fd_power_home >= 0) { close(fd_power_home); }
    if (fd_tbtn >= 0) { close(fd_tbtn); }
    if (fd_power >= 0) { close(fd_power); }
    if (fd_tscreen >= 0) { close(fd_tscreen); }
}

int touch_add_area(const struct touch_area *area)
{
    printf("(%s) ----- pointer --> %p\n", __func__, (void*)area);
    if (!area) return -1;

    // Look for existing area with same id
    for (int i = 0; i < areas_count; i++) {
        if (areas[i].id == area->id) {
            areas[i] = *area;    // replace
            return 1;
        }
    }

    // No match append
    if (areas_count >= MAX_TOUCH_AREAS)
        return -1;             // table full

    areas[areas_count++] = *area;
    printf("(%s) areas_count: %d\n", __func__, areas_count);
    return 0;
}

void touch_clear_areas(void)
{
    areas_count = 0;
    printf("(%s) areas_count: %d\n", __func__, areas_count);
}

int touch_update_areas(const struct touch_area *new_areas, int count)
{
    if (count < 0 || count > MAX_TOUCH_AREAS)
        return -1;

    for (int i = 0; i < count; i++) {
        areas[i] = new_areas[i];
    }
    areas_count = count;
    return 0;
}

int touch_remove_area(int id)
{
    for (int i = 0; i < areas_count; i++) {
        if (areas[i].id != id) continue;

        // Shift everything after it down by one
        for (int j = i; j < areas_count - 1; j++) {
            areas[j] = areas[j + 1];
        }
        areas_count--;
        return 0;
    }
    return -1;                 // not found
}
