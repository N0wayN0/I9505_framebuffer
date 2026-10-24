#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/msm_mdp.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include "shapes.h"
#include "fb_only.h"
#include "fonts_only.h"
#include "input_events.h"
#include "overlay.h"
#include "layout.h"
#include "timer.h"

int main() {

    init_fb();
    init_overlays();
    unset_all_overlays();
    fb_swap();
    input_init();
    init_timers();
    set_theme("pic2.jpg");
    //timer_add(0, 5000, get_timestamp, 0);
    //timer_add(2000, 0, fb_swap, 7);
    print_timer(0);
    print_timer(1);
    print_timer(2);
    //exit(0);
    //add_bitmap_to_ion("pic2.jpg"); // blits goes here for testing
    //clear_half();  // blank white half of pic for blit test


    //---------------- LAYOYT -------------------------
    extern app1_layout1;
    extern app1_layout2;
    printf("(%s) ----- struct pointer app1_layout_1 --> %p  \n", __func__, (void *)&app1_layout1);
    create_layout(&app1_layout1);
    fb_swap();
    // display image blitted test
     //       int ov_slot = prepare_overlay2("pic2.jpg");
       //     int ov_id = display_overlay2(ov_slot, 1);
    
while (1) {
    struct touch_area hit;
    //struct pollfd fds[8];
    int nfds = 0;
    int timeout = 50;
    //nfds = input_get_fds(&fds, 8);
    nfds = input_get_fds(8);
    //printf("(%s) no of fds: %d\n", __func__, nfds);
    //int r = input_poll(&hit, nfds, timeout);
    int r = input_poll(&hit, nfds, timer_next_ms());
    if (r == 1) {   // 1 - input event occured
        printf("------------ NEW CLICK EVENT ----------\n");
        printf("(%s) clicked id=%d action=%s\n",__func__, hit.id, hit.action);
        execute(hit.id);
        //hexdump(&app1_layout1,96);
    } else if (r == 0) {  // 0 - timeout
        timer_check();                  /* fire anything due */
    }

}

//exit(0);



unset_all_overlays();
exit(6);




// draw 3 horizontal lines
for (int i = 100; i < 1000; i++) {
    put_pixel(i, 200, "#ff0000");
    put_pixel(i, 201, "#ff0000");
    put_pixel(i, 202, "#ff0000");
    put_pixel(i, 210, "#00ff00");
    put_pixel(i, 211, "#00ff00");
    put_pixel(i, 212, "#00ff00");
    put_pixel(i, 220, "#0000ff");
    put_pixel(i, 221, "#0000ff");
    put_pixel(i, 222, "#0000ff");
}
fb_swap();
sleep(2);

    draw_text("Ready",300,850,"#ffffff",3);
    fb_swap();
    sleep(2);

draw_rect(620,90,88,55,8, "#0FE60B");
draw_rect_frame(220,1390,378,355,8, "#0FE6aaB");
draw_rounded_rect(599, 200, 200, 200, 40, "#ff0000");
fb_swap();
sleep(2);

draw_circle(400, 800, 160, "#ffffff");
fb_swap();
sleep(2);

fill_rect(420,690,488,500, "#a80fb1");
drawline(300,300, 600, 600, "#ff0000");
fb_swap();
sleep(2);

drawline(600,600, 922, 1555, "#00ff00");
drawline(700,170, 822, 1920, "#0000ff");
draw_rect(620,90,88,55,8, "#0FE60B");
draw_v_line(300, 800, 60, "#ff003b");
draw_v_line(301, 800, 160, "#ff003b");
draw_v_line(302, 800, 60, "#ff003b");
draw_h_line(300, 800, 60, "#ff003b");
fb_swap();
sleep(2);
    
draw_text("KONIEC",300,850,"#ffffff",3);
fb_swap();
sleep(1);

close_fb();
return 0;
}
