#ifndef SHAPES_H
#define SHAPES_H

void draw_v_line(int x, int y, int h, const char *rgb);
void draw_h_line(int x, int y, int w, const char *rgb);
void draw_rect(int x, int y, int w, int h, int thick, const char *rgb);
void fill_rect(int x, int y, int w, int h, const char *rgb);
void drawline(int x0, int y0, int x1, int y1, const char *hex_rgb);
void draw_rect_frame(int x, int y, int w, int h, int b, const char *hex_rgb);
void plot4points(int cx, int cy, int x, int y, const char *pixel);
void draw_circle(int cx, int cy, int radius, const char *pixel);
void draw_rounded_rect(int x, int y, int w, int h, int r, const char *color);

#endif
