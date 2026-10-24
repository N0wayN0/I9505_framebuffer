#ifndef LAYOUT_H
#define LAYOUT_H

void create_layout(const struct layout *layout);
void refresh_layout();
void execute(int id);

    
enum element_type {
    ELEM_BACKGROUND,
    ELEM_STATIC,
    ELEM_LABEL,
    ELEM_TEXTVIEW,
    ELEM_BUTTON,
    ELEM_CHECKBOX,
    ELEM_RADIO,
    ELEM_SLIDER,
};


const struct element {
    int  id;
    int  type;

    int  x, y, w, h;

    char label[16];      // used by LABEL, TEXTVIEW, BUTTON, CHECKBOX, RADIO
    char value[16];      // used by TEXTVIEW, SLIDER (current value as text)
    char color[8];

    int  clickable;
    char action[64];      // used by BUTTON, CHECKBOX, RADIO, SLIDER
    void (*on_click)();   // click handling function
    int  state;           // used by CHECKBOX, RADIO (0/1), SLIDER (position)

    // range
    int  min, max;        // used by SLIDER only

    // styling (optional)
    int  border;          // used by TEXTVIEW, BUTTON
    int  font_size;       // used by LABEL, TEXTVIEW, BUTTON
    int  align;           // 0=left, 1=center, 2=right

    // rendering (later)
    int  mem_fd;          // ION buffer with pixel data
};

struct layout {
    char *name;
    const struct element *elements;
    int count;
};

#endif
