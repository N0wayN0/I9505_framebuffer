#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

// display
/*int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
uint8_t *fbp = 0;
uint8_t* current_buffer = 0;
uint8_t* back_buffer = 0;
int current_page = 0;
int page_size = 0;
long int location = 0;
int pinc = 0;
int count ;
int page_size = 0;
int cur_page = 0;
int back_buffer_y = 0;
struct mdp_display_commit commit;
int ret; */

//#define FB_OFF(fbx,fby) ((fbx+vinfo.xoffset)*pinc+(fby+vinfo.yoffset)*finfo.line_length)
#define FB_OFF(fbx,fby) ((fbx+vinfo.xoffset)*pinc+(fby+back_buffer_y)*finfo.line_length)



void put_pixel(int x, int y, const char *hex_rgb);
void fb_swap();
int init_fb();
void close_fb();

#endif
