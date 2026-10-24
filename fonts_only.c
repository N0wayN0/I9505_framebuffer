#include "fb_only.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
//#include "roboto_10x18.h"
#include "roboto_23x41.h"


// Structure to hold a character bitmap
typedef struct {
    uint8_t* data;      // Pixel data (1 byte per pixel, 0=background, 1=foreground)
    int width;          // Character width (should be font.cwidth)
    int height;         // Character height (should be font.cheight)
    int pitch;          // Bytes per row (width, for alignment)
} CharBitmap;



//------------------------------------------------- FONTS ---------------------------
// Extract a character from the font and return as bitmap
CharBitmap* extract_char_bitmap(char c) {
    // Validate character (ASCII 32-127)
    if (c < 32 || c > 127) return NULL;
    
    // Allocate the bitmap structure
    CharBitmap* bitmap = malloc(sizeof(CharBitmap));
    if (!bitmap) return NULL;
    
    // Set dimensions
    bitmap->width = font.cwidth;
    bitmap->height = font.cheight;
    bitmap->pitch = font.cwidth;
    
    // Allocate memory for pixel data
    int total_pixels = font.cwidth * font.cheight;
    bitmap->data = malloc(total_pixels);
    if (!bitmap->data) {
        free(bitmap);
        return NULL;
    }
    
    // Initialize to 0 (background)
    memset(bitmap->data, 0, total_pixels);
    
    // Calculate character position in the strip
    int char_index = c - 32;
    int start_x = char_index * font.cwidth;
    
    // Decode the font and extract the character
    unsigned char* in = font.rundata;
    unsigned char data;
    int pixel_pos = 0;
    
    while ((data = *in++)) {
        int run_length = data & 0x7f;
        uint8_t pixel_color = (data & 0x80) ? 1 : 0;
        
        for (int i = 0; i < run_length; i++) {
            // Calculate position in the full bitmap
            int row = pixel_pos / font.width;
            int col = pixel_pos % font.width;
            
            // Only extract if this pixel belongs to our character
            if (col >= start_x && col < start_x + font.cwidth) {
                int char_col = col - start_x;
                int char_row = row;
                int idx = char_row * font.cwidth + char_col;
                bitmap->data[idx] = pixel_color;
            }
            pixel_pos++;
        }
    }
    
    return bitmap;
}

// Free the character bitmap
void free_char_bitmap(CharBitmap* bitmap) {
    if (bitmap) {
        if (bitmap->data) free(bitmap->data);
        free(bitmap);
    }
}

// scale font
CharBitmap* scale_bitmap(const CharBitmap* src, int scale_factor) {
    if (!src || scale_factor < 1) {
        return NULL;
    }

    if (scale_factor == 1) {
        // Just copy the bitmap
        CharBitmap* dst = (CharBitmap*)malloc(sizeof(CharBitmap));
        if (!dst) return NULL;

        dst->width = src->width*scale_factor;
        dst->height = src->height;
        dst->pitch = src->width;
        dst->data = (uint8_t*)malloc(src->width * src->height);
        if (!dst->data) {
            free(dst);
            return NULL;
        }
        memcpy(dst->data, src->data, src->width * src->height);
        return dst;
    }

   // Calculate new dimensions
    int new_width = src->width * scale_factor;
    int new_height = src->height * scale_factor;

    // Allocate destination bitmap
    CharBitmap* dst = (CharBitmap*)malloc(sizeof(CharBitmap));
    if (!dst) return NULL;

    dst->width = new_width;
    dst->height = new_height;
    dst->pitch = new_width;
    dst->data = (uint8_t*)calloc(new_width * new_height, 1); // Zero-initialize
    if (!dst->data) {
        free(dst);
        return NULL;
    }

    // Scale up using nearest neighbor interpolation
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            uint8_t pixel = src->data[y * src->pitch + x];

            // Fill the scaled block
            int start_y = y * scale_factor;
            int start_x = x * scale_factor;

            for (int dy = 0; dy < scale_factor; dy++) {
                for (int dx = 0; dx < scale_factor; dx++) {
                    dst->data[(start_y + dy) * new_width + (start_x + dx)] = pixel;
                }
            }
        }
    }

    return dst;
}
    
// Draw a character bitmap at position (x, y) with given color
void draw_char_bitmap(int x, int y, CharBitmap* bitmap, const char *color) {
    if (!bitmap || !bitmap->data) return;
    
    for (int row = 0; row < bitmap->height; row++) {
        for (int col = 0; col < bitmap->width; col++) {
            int idx = row * bitmap->pitch + col;
            if (bitmap->data[idx]) {  // Foreground pixel
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

// Draw a character directly (extract + draw in one step)
int draw_char(int x, int y, char c, const char *color,int factor) {
    CharBitmap* bitmap = extract_char_bitmap(c);
    if (bitmap) {
        CharBitmap* scaled = scale_bitmap(bitmap, factor);
        draw_char_bitmap(x, y, scaled, color);
        free_char_bitmap(bitmap);
        free_char_bitmap(scaled);
    }
    return factor;
}

void draw_text(const char* text, int x, int y, const char* color, int scale_factor) {
    if (!text || scale_factor < 1) {
        return;
    }

    int current_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n') { // enter, new line
            y += (font.cheight * scale_factor);
            current_x = x;
        } else {
            int used_scale = draw_char(current_x, y, text[i], color, scale_factor);
            //printf("x: %d  y: %d   %x\n" ,current_x,y,text[i]);
            current_x += (font.cwidth * used_scale);
        }
    }
}

//--------------------------------------------END OF FONTS ---------------------------

/*
    draw_text("Ready",300,850,"#ffffff",3);
    draw_text("Siema",20,150,"#ffffff",5);
    
    CharBitmap* char_A = extract_char_bitmap('A');
    CharBitmap* scaled = scale_bitmap(char_A, 3);
    if (char_A) {
        draw_char_bitmap(500, 200, char_A, "#ff0000");    // Red
        draw_char_bitmap(500, 250, char_A, "#00ff00");    // Green
        draw_char_bitmap(500, 300, char_A," #0060ff");    // Blue
        draw_char_bitmap(500, 350, scaled," #ffffff");    // white
        free_char_bitmap(char_A);
        free_char_bitmap(scaled);
    }
    
    // Method 2: Draw directly (extracts and draws in one go)
    draw_char(200, 510, 'R', "#ff0000", 2);    // Red
    draw_char(260, 510, 'G', "#00ff00", 2);    // Green
    draw_char(320, 510, 'B', "#0000ff", 2);    // Blue
    draw_char(380, 510, 'W', "#ffffff", 2);    // white
    
*/
