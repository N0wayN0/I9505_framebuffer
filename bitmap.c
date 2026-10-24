#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "overlay.h"  // needs to know about memPicture
#include <stdio.h>

unsigned char* get_bitmap(struct memPicture *meminfo) {
//    printf("\n(%s) ----- struct pointer --> %p\n", __func__, (void *)meminfo);
    int width, height, channels;

//    printf("(%s) mem_info->name: %s\n", __func__, meminfo->name);
    unsigned char *data = NULL;
    // stbi_info returns 1 on success, 0 on failure
    if (data = stbi_load(meminfo->name, &width, &height, &channels, 4)) {
        size_t size = (size_t)width * height * 4;
        meminfo->size = size;
        meminfo->width = width;
        meminfo->height = height;
//        printf("(%s) mem_info->width:   %d\n", __func__, meminfo->width);
//        printf("(%s) mem_info->height:  %d\n", __func__, meminfo->height);
//        printf("(%s) mem_info->size:    %d\n", __func__, meminfo->size);
        //printf("Channels: %d\n", channels);
    } else {
        // Get the reason for failure
        const char *reason = stbi_failure_reason();
        printf("Failed to read image info: %s\n", reason);
    }

    return data;
}
