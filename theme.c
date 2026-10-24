#include "overlay.h"
#include <stdio.h>

int set_theme(const char *name) {
    printf("\n(%s) Load theme: %s\n",__func__, name);
    int ret = add_bitmap_to_ion(name);
    return ret;
}

int clear_theme() {

}
