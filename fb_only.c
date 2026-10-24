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
#include "fb_only.h"

int fbfd = 0;
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
int ret;

void put_pixel(int x, int y, const char *hex_rgb)
{
    char hex_red[3], hex_green[3], hex_blue[3], hex_alpha[3];
    unsigned int pix_offset = 0 ;x + y * finfo.line_length;
    pix_offset = FB_OFF(x,y);
    /* split color string */
    // Skip leading '#' if present
    if (hex_rgb[0] == '#') {
        hex_rgb++;
    }
    
    // Red
    hex_red[0] = hex_rgb[0];
    hex_red[1] = hex_rgb[1];
    hex_red[2] = '\0';
    // Green
    hex_green[0] = hex_rgb[2];
    hex_green[1] = hex_rgb[3];
    hex_green[2] = '\0';
    // Blue
    hex_blue[0] = hex_rgb[4];
    hex_blue[1] = hex_rgb[5];
    hex_blue[2] = '\0';
    *(fbp + pix_offset    ) = (unsigned char)strtol(hex_red, NULL, 16);
    *(fbp + pix_offset + 1) = (unsigned char)strtol(hex_green, NULL, 16);
    *(fbp + pix_offset + 2) = (unsigned char)strtol(hex_blue, NULL, 16);
}

// Swap buffers
void fb_swap() {
//    printf("++++++++++++++++++\n"); 
    back_buffer = current_buffer;
    back_buffer_y = vinfo.yoffset;
//    printf("(%s) Get vinfo.yoffset : %d\n" ,__func__,  vinfo.yoffset);
    current_page = (current_page + 1) % 2;
//    printf("(%s) Set page to: %p\n", __func__,  current_page); 
    //             addres      0 or 1        1920 (screen height)
    current_buffer = fbp + (current_page * page_size);
    printf("(%s) Set buffer to: %p\n", __func__,  current_buffer); 
    
    // Show current page
    vinfo.yoffset = current_page * vinfo.yres;
//    printf("(%s) Set vinfo.yoffset : %d\n", __func__,  vinfo.yoffset);
    ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
//    printf("(%s) Showing yoffset : %d\n", __func__,  vinfo.yoffset);
//    printf("(%s) clear buffer: %p\n", __func__,  back_buffer); 
    memset(back_buffer, 0, page_size);
//    if (vinfo.yoffset == 0) {
//        draw_text("1",900,50,"#ffffff",5);
//    } else {
//        draw_text("2",900,50,"#ffffff",5);
//    }
}


int init_fb()
{
  memset( &commit, 0, sizeof commit );

  /* Open the file for reading and writing */
  fbfd = open("/dev/graphics/fb0", O_RDWR);
  if (!fbfd) {
    printf("Error: cannot open framebuffer device.\n");
    exit(1);
  }
  printf("The framebuffer device was opened successfully.\n");
  /* Get fixed screen information */
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Error reading fixed information.\n");
    exit(2);
  }

  /* Get variable screen information */
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Error reading variable information.\n");
    exit(3);
  }
  vinfo.activate = FB_ACTIVATE_FORCE;
  commit.flags |=  MDP_DISPLAY_COMMIT_OVERLAY;
  commit.var = vinfo;
  if (ioctl(fbfd, MSMFB_DISPLAY_COMMIT, &commit)) {
    printf("Error writing commit information.\n");
    exit(4);
  }
  vinfo.yoffset = 0;
  vinfo.xoffset = 0;
  if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
    printf("Error writing variable information.\n");
    exit(5);
  }

  /* Figure out the size of the screen in bytes */
  screensize = vinfo.xres_virtual * vinfo.yres_virtual * vinfo.bits_per_pixel / 8;
  printf("Memory size: %d\n",screensize);
  printf("vinfo.bpp = %d\n",vinfo.bits_per_pixel);
  printf("vinfo.xres = %d\n",vinfo.xres);
  printf("vinfo.yres = %d\n",vinfo.yres);

  page_size = finfo.smem_len / 2;
  /* Map the device to memory */
  fbp = (uint8_t *)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED,fbfd, 0);
  if ((int)fbp == -1) {
    printf("Error: failed to map framebuffer device to memory.\n");
    exit(6);
  }
  printf("The framebuffer was mapped to memory successfully.\n");
  current_buffer = fbp;
  back_buffer = fbp + page_size;
  pinc = vinfo.bits_per_pixel / 8;
  printf("currBuf: %p\n", (void*)current_buffer);
  printf("backBuf: %p\n", (void*)back_buffer);
  //printf("pageSize: %d\n", page_size);

  memset( fbp, 0, finfo.smem_len );
} 

void close_fb() {
    munmap(fbp, finfo.smem_len);
    close(fbfd);
}
