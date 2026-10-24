#ifndef FONTS_ONLY_H
#define FONTS_ONLY_H

// Structure to hold a character bitmap
typedef struct {
    uint8_t* data;      // Pixel data (1 byte per pixel, 0=background, 1=foreground)
    int width;          // Character width (should be font.cwidth)
    int height;         // Character height (should be font.cheight)
    int pitch;          // Bytes per row (width, for alignment)
} CharBitmap;



CharBitmap* extract_char_bitmap(char c);
void free_char_bitmap(CharBitmap* bitmap);
CharBitmap* scale_bitmap(const CharBitmap* src, int scale_factor);
void draw_char_bitmap(int x, int y, CharBitmap* bitmap, const char *color);
int draw_char(int x, int y, char c, const char *color,int factor);
void draw_text(const char* text, int x, int y, const char* color, int scale_factor);

#endif
