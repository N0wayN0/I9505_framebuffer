#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/msm_mdp.h>
#include <linux/msm_ion.h>
#include "overlay.h"  // has to be just after ion. So memPicture inside array wont be corrupted
#include "layout.h" // blit neets to know about element struct
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#define MDP_V4_0 40
#define ALIGN(x, align) (((x) + ((align)-1)) & ~((align)-1))

typedef uint8_t GGLubyte;		// ub
int ion_fd = -1;
extern int fbfd;

#define MAX_OVERLAYS 5
#define MAX_PICTURES 8
static struct memPicture pictures[MAX_PICTURES] = {0};
static struct mdp_overlay overlays[MAX_OVERLAYS] = {0};

static struct lista blit_list = {0};
int MAX_BLITS = 6;  // tyle samo co w lista w overlay.h
//static struct mdp_blit_req_list *blit_list = NULL;

unsigned char* read_file_mmap(const char* filename, size_t* out_size) {
    int fd;
    struct stat st;
    unsigned char* data = NULL;

    // Open file
    printf("\n(%s) Try open file: %s\n", __func__,filename);
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open file\n");
        return NULL;
    }
    printf("(%s) File opened succesfully\n", __func__);

    // Get file size
    if (fstat(fd, &st) != 0) {
        perror("Failed to get file stats");
        close(fd);
        return NULL;
    }
    printf("(%s) File size: %d\n\n", __func__, st.st_size);

    // Map file to memory
    data = (unsigned char*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("Failed to mmap file");
        close(fd);
        return NULL;
    }

    // Store size
    if (out_size) {
        *out_size = st.st_size;
    }

    // Note: You need to call munmap() to free this, not free()
    close(fd);
    return data;
}

void init_overlays(void) {
    for (int i = 0; i < MAX_OVERLAYS; i++) {
        overlays[i].id = -1;
        printf("(%s) overlays[%d].id set to %d\n", __func__, i, overlays[i].id);
    }
}

void print_memory_info(struct memPicture *mem) {
    printf("(%s) print memPicture\n", __func__);
    printf("mem_buf       %p\n", mem->mem_buf);
    printf("size:         %u\n", mem->size);
    printf("mem_fd:       %u\n", mem->mem_fd);
    //printf("filename:     %s\n", mem->filename);
    printf("name:         %s\n", mem->name);
    printf("width:        %d\n", mem->width);
    printf("height:       %d\n", mem->height);
    printf("data.handle   %p\n", mem->handle_data.handle);
}

void print_overlay_info(const struct mdp_overlay *ov) {
    printf("\n======== Requested Overlay Info ========\n");
    printf("(%s) ----- struct pointer --> %p \n", __func__, (void *)ov);
    printf(" overlay.src.width      = %u\n", ov->src.width);
    printf(" overlay.src.height     = %u\n", ov->src.height);
    printf(" overlay.src.format     = %u\n", ov->src.format);
    printf(" overlay.src_rect.x     = %u\n", ov->src_rect.x);
    printf(" overlay.src_rect.y     = %u\n", ov->src_rect.y);
    printf(" overlay.src_rect.w     = %u\n", ov->src_rect.w);
    printf(" overlay.src_rect.h     = %u\n", ov->src_rect.h);
    printf(" overlay.dst_rect.x     = %u\n", ov->dst_rect.x);
    printf(" overlay.dst_rect.y     = %u\n", ov->dst_rect.y);
    printf(" overlay.dst_rect.w     = %u\n", ov->dst_rect.w);
    printf(" overlay.dst_rect.h     = %u\n", ov->dst_rect.h);
    printf(" overlay.z_order        = %u\n", ov->z_order);
    printf(" overlay.is_fg          = %u\n", ov->is_fg);
    printf(" overlay.alpha          = %u\n", ov->alpha);
    printf(" overlay.transp_mask    = %u\n", ov->transp_mask);
    printf(" overlay.blend_op       = %u\n", ov->blend_op);
    printf(" overlay.flags          = 0x%X\n", ov->flags);
    printf(" overlay.id             = %d\n", ov->id);
//    printf(" overlay.user_data[0-7] = %u, %u, %u, %u, %u, %u, %u, %u\n",
//           ov->user_data[0], ov->user_data[1], ov->user_data[2],
//           ov->user_data[3], ov->user_data[4], ov->user_data[5],
//           ov->user_data[6], ov->user_data[7]);
//
    // The overlay_pp_cfg is itself a nested structure.
    // You can add another function to print that if needed.
    // printf("  overlay_pp_cfg ...\n");
}

void print_blit_req(const struct mdp_blit_req *req) {
    printf("\n======== Print Blit Info ========\n");
    printf("(%s) ----- struct pointer --> %p \n", __func__, (void *)req);
    printf(" req.src.width      = %u\n", req->src.width);
    printf(" req.src.height     = %u\n", req->src.height);
    printf(" req.src.format     = %u\n", req->src.format);
    printf(" req.src.memory_id  = %u\n", req->src.memory_id);
    printf(" req.dst.width      = %u\n", req->dst.width);
    printf(" req.dst.height     = %u\n", req->dst.height);
    printf(" req.dst.format     = %u\n", req->dst.format);
    printf(" req.dst.memory_id  = %u\n", req->dst.memory_id);
    printf(" req.src_rect.x     = %u\n", req->src_rect.x);
    printf(" req.src_rect.y     = %u\n", req->src_rect.y);
    printf(" req.src_rect.w     = %u\n", req->src_rect.w);
    printf(" req.src_rect.h     = %u\n", req->src_rect.h);
    printf(" req.dst_rect.x     = %u\n", req->dst_rect.x);
    printf(" req.dst_rect.y     = %u\n", req->dst_rect.y);
    printf(" req.dst_rect.w     = %u\n", req->dst_rect.w);
    printf(" req.dst_rect.h     = %u\n", req->dst_rect.h);
    printf(" req.alpha          = %u\n", req->alpha);
    printf(" req.transp_mask    = %u\n", req->transp_mask);
    printf(" req.flags          = 0x%08x\n", req->flags);
}

int unset_all_overlays() {
    int ret;
    struct msmfb_mixer_info_req mixer_req = {0};
    struct mdp_display_commit commit = {0};
    
    // 1. Query the current mixer configuration for
    //    list of all overlay pipes currently in use
    mixer_req.mixer_num = 0; // 0 is the primary display
    ret = ioctl(fbfd, MSMFB_MIXER_INFO, &mixer_req);
    if (ret < 0) {
        printf("(%s) MSMFB_MIXER_INFO failed\n", __func__);
        return -1;
    }
    
    printf("(%s) Found %d active overlay pipes.\n", __func__, mixer_req.cnt);
    
    // 2. Unset each active pipe
    for (int i = 0; i < mixer_req.cnt; i++) {
        int pipe_id = mixer_req.info[i].pndx;
        if (pipe_id == 5) continue;
        printf("(%s) Unsetting pipe: %d (z_order: %d)\n", 
               __func__, pipe_id, mixer_req.info[i].z_order);
        
        ret = ioctl(fbfd, MSMFB_OVERLAY_UNSET, &pipe_id);
        if (ret < 0) {
            printf("(%s) MSMFB_OVERLAY_UNSET failed\n", __func__);
            // Continue trying to clean up other pipes even if one fails
        }
    }
    
    // 3. Commit the changes to the display hardware
    commit.flags = MDP_DISPLAY_COMMIT_OVERLAY;
    commit.wait_for_finish = 1;
    
    ret = ioctl(fbfd, MSMFB_DISPLAY_COMMIT, &commit);
    if (ret < 0) {
        printf("(%s) MSMFB_DISPLAY_COMMIT failed\n", __func__);
        return -1;
    }
   
    init_overlays(); 
    printf("(%s) All overlays cleared and committed.\n", __func__);
    return 0;
}

int target_has_overlay(char *version) {
    int ret;
    int mdp_version;
    int overlay_supported = 0;

    if (strlen(version) >= 8) {
        if(!strncmp(version, "msmfb", strlen("msmfb"))) {
            char str_ver[4];
            memcpy(str_ver, version + strlen("msmfb"), 3);
            str_ver[3] = '\0';
            mdp_version = atoi(str_ver);
            if (mdp_version >= MDP_V4_0) {
                overlay_supported = 1;
            }
        }
    }

    return overlay_supported;
}

int free_ION_mem(struct memPicture *mem) {
    if (!mem->mem_buf) return 0;

    int ret = 0;
//    printf("\n(%s) remove memPicture.mem_buf\n",__func__);
    munmap(mem->mem_buf, mem->size);
//    printf("(%s) unmapped %d bytes\n",__func__,mem->size);
//    printf("(%s) at %p\n",__func__,mem->mem_buf);

    if (ion_fd >= 0) {
//        printf("(%s) ion_fd: %d\n",__func__,ion_fd);
        struct ion_handle_data handle_data;
        handle_data.handle = mem->handle_data.handle;
        ret = ioctl(ion_fd, ION_IOC_FREE, &handle_data);
        if (ret < 0)
            printf("free_mem failed\n");
//        printf("(%s) removed handle %p\n",__func__,mem->handle_data.handle);
    }

    if (mem->mem_fd >= 0) {
        close(mem->mem_fd);
//        printf("(%s) mem_fd %d closed\n",__func__, mem->mem_fd);
    }

    memset(mem, 0, sizeof(struct memPicture));
    printf("(%s) Data in ION memory cleared\n",__func__);
//    print_memory_info(mem);
    return 0;
}

int alloc_ION_mem(struct memPicture *ion_mem)
{
    printf("\n(%s) ----- struct pointer --> %p \n", __func__, (void *)ion_mem);
    int size = ion_mem->size;
    int result;
    struct ion_fd_data fd_data;
    struct ion_allocation_data ionAllocData;
    memset(&ionAllocData, 0, sizeof(ionAllocData));

    if (ion_fd < 0 ) {
        ion_fd = open("/dev/ion", O_RDWR|O_DSYNC);
        if (ion_fd < 0) {
            printf("ERROR: Can't open ion\n");
            return;
        }
//        printf("(%s) ION openned. Success\n",__func__);
    }
//    printf("(%s) ion_fd: %d\n",__func__,ion_fd);

    ionAllocData.flags = 0;
    ionAllocData.len = size;
    ionAllocData.align = sysconf(_SC_PAGESIZE);
    ionAllocData.heap_mask = ION_HEAP(ION_HEAP_TYPE_CARVEOUT);
    ionAllocData.heap_mask = ION_HEAP(ION_SYSTEM_CONTIG_HEAP_ID);
    ionAllocData.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
    // only one which works
    ionAllocData.heap_mask = ION_HEAP(ION_IOMMU_HEAP_ID);
    //printf("\npagesize %d\n",ionAllocData.align);

//    printf("(%s) ION_IOC_ALLOC will try to allocate: %d bytes\n", __func__ , size);
    result = ioctl(ion_fd, ION_IOC_ALLOC, &ionAllocData);
    if(result){
        printf("\n(%s) ION_IOC_ALLOC Filed\n", __func__);
        close(ion_fd);
        exit(5);
    }
//    printf("(%s) ION_IOC_ALLOC handle: %p\n", __func__, ionAllocData.handle);
    printf("(%s) ION Allocated: %d bytes of memory\n", __func__, size);

    fd_data.handle = ionAllocData.handle;
    ion_mem->handle_data.handle = ionAllocData.handle;
//    printf("(%s) fd_data.fd before: %d\n",__func__,fd_data.fd);
    //result = ioctl(ion_fd, ION_IOC_MAP, &fd_data);
    result = ioctl(ion_fd, ION_IOC_SHARE, &fd_data);
    if (result) {
        printf("\n(%s) ION_IOC_MAP Filed\n",__func__);
        free_ION_mem(ion_mem);
        exit(6);
    }
//    printf("(%s) ION_IOC_MAP OK. res: %d\n",__func__,result);
//    printf("(%s) fd_data.fd after: %d\n",__func__,fd_data.fd);
//    printf("(%s) fd_data.handle: %p\n", __func__, fd_data.handle);
//    printf("(%s) size: %d\n", __func__, size);
    ion_mem->mem_buf = (unsigned char *)mmap(NULL, size, PROT_READ |
                PROT_WRITE, MAP_SHARED, fd_data.fd, 0);

    if (!ion_mem->mem_buf) {
        printf("ERROR: mem_buf MAP_FAILED ");
        free_ION_mem(ion_mem);
        exit(7);
    }
//    printf("(%s) mmap() -> mem_buf: %p\n", __func__, ion_mem->mem_buf);
    ion_mem->mem_fd = fd_data.fd;

    return 0;
}

int prepare_overlay2(const char *name) {
    printf("\n");
    int slot = -1;
    struct mdp_overlay *overlay;
    for (int i = 0; i < MAX_OVERLAYS; i++) {
        overlay = &overlays[i];
//        printf("(%s) check overlays[%d].id for empty slot == (-1) \n", __func__, i);
//        printf("(%s) check overlays[%d].id = %d\n", __func__, i, overlay->id);
        if (overlay->id == -1) {
            printf("(%s) overlays[%d] is empty\n", __func__, i);
            slot = i;
            break;
        }
    }
    for (int i = 0; i <= MAX_PICTURES; i++) {
        struct memPicture *pic = &pictures[i];
        printf("(%s) check pictures[%d].name=%s == %s\n", __func__, i, pic->name, name);
        //print_memory_info(pic);
        if ((strcmp(pic->name, name)) == 0 ) {
            printf("(%s) Found pictures[%d].name == %s \n", __func__, i, name);
//            print_memory_info(pic);
            printf("(%s) Sourcing image details\n", __func__);
            //printf("(%s) pic->name %s \n", __func__, &pic->name);
            // source details
            overlay->src.format = MDP_RGBA_8888;
            overlay->src.width = pic->width;
            overlay->src.height = pic->height;
            // crop
            overlay->src_rect.x = 0;
            overlay->src_rect.y = 0;
            overlay->src_rect.w = pic->width;
            overlay->src_rect.h = pic->height;
            // destination
            overlay->dst_rect.x = 0;
            overlay->dst_rect.y = 0;
            overlay->dst_rect.w = 1080;
            overlay->dst_rect.h = 1920;
            // config
            overlay->alpha = 0xaf;
            overlay->is_fg = 1;
            overlay->z_order = slot; // one overlay on one plane
            overlay->transp_mask = MDP_TRANSP_NOP;
            overlay->id = -2;  // mark as initial setup done
            break;
        }
    }
    return slot;
}
/*
void prepare_overlay(struct mdp_overlay *overlay, int width, int height) {
    printf("\n(%s) ----- struct pointer --> %p \n", __func__, (void *)overlay);
    printf("(%s) width %d, height %d\n", __func__, width, height);
    // source details
    overlay->src.format = MDP_RGBA_8888;
    overlay->src.width = width;
    overlay->src.height = height;
    // crop
    overlay->src_rect.x = 0;
    overlay->src_rect.y = 0;
    overlay->src_rect.w = width;
    overlay->src_rect.h = height;
    // destination
    overlay->dst_rect.x = 0;
    overlay->dst_rect.y = 0;
    overlay->dst_rect.w = 1080;
    overlay->dst_rect.h = 1920;
    // config
    overlay->alpha = 0xaf;
    overlay->is_fg = 1;
    overlay->z_order = 3;
    overlay->transp_mask = MDP_TRANSP_NOP;
    overlay->id = MSMFB_NEW_REQUEST;
}
*/
int crop_overlay2(int x, int y, int w, int h) {
    //printf("\n(%s) ----- struct pointer --> %p \n", __func__, (void *)overlay);
    int slot = -1;
    struct mdp_overlay *overlay;
    for (int i = 0; i < 5; i++) {
        overlay = &overlays[i];
//        printf("(%s) check overlays[%d] for id == -2 \n", __func__, i);
        if (overlay->id == -2) {
            printf("(%s) overlays[%d] ready for crop\n", __func__, i);
            slot = i;
            break;
        }
    }
    printf("(%s) x: %d, y: %d, w: %d, h: %d\n", __func__, x, y, w, h);
    overlay->src_rect.x = x;
    overlay->src_rect.y = y;
    overlay->src_rect.w = w;
    overlay->src_rect.h = h;
    return slot;
}

int paste_overlay2(int x, int y, int w, int h) {
    //printf("\n(%s) ----- struct pointer --> %p \n", __func__, (void *)overlay);
    int slot = -1;
    struct mdp_overlay *overlay;
    for (int i = 0; i < MAX_OVERLAYS; i++) {
        overlay = &overlays[i];
//        printf("(%s) check overlays[%d] for id == -2 \n", __func__, i);
        if (overlay->id == -2) {
            printf("(%s) overlays[%d] ready for destination\n", __func__, i);
            slot = i;
            break;
        }
    }
    printf("(%s) x: %d, y: %d, w: %d, h: %d\n", __func__, x, y, w, h);
    overlay->dst_rect.x = x;
    overlay->dst_rect.y = y;
    overlay->dst_rect.w = w;
    overlay->dst_rect.h = h;
    return slot;
}

//allocate overlay and display
int display_overlay2(int ov_slot, int ion_slot) {
    printf("\n(%s) overlay[%d]   pictures[%d]\n",__func__, ov_slot, ion_slot);
    if (ion_slot < 0) {
        printf("(%s) Error. Picture not loaded -1\n",__func__);
        struct mdp_overlay *overlay = &overlays[ov_slot];
        memset(overlay, 0, sizeof(struct mdp_overlay));
        overlay->id = -1;
        //print_overlay_info(overlay);
        return -1;
    }
    int ret;
    struct mdp_overlay *overlay = &overlays[ov_slot];
    overlay->id = MSMFB_NEW_REQUEST;
//    print_overlay_info(overlay);
    ret = ioctl(fbfd, MSMFB_OVERLAY_SET, overlay);
    if (ret < 0 ) {
        printf("(%s) MSMFB_OVERLAY_SET failed\n\n",__func__, ret);
        return ret;
    }
    printf("(%s) OVERLAY_SET Success\n",__func__);
    printf("(%s) overlay.id=%d\n",__func__, overlay->id);
    
    ret = 0;
    struct msmfb_overlay_data ovdata = {0};
    ovdata.id = overlay->id;
    struct memPicture *pic = &pictures[ion_slot];
    ovdata.data.memory_id = pic->mem_fd;
    ovdata.data.offset = 0;
    ovdata.data.flags = 0;
    
    ret = ioctl(fbfd, MSMFB_OVERLAY_PLAY, &ovdata);
    if (ret < 0) {
        printf("(%s) MSMFB_OVERLAY_PLAY failed\n\n", __func__);
        return ret;
    }
    struct mdp_display_commit commit;
    commit.flags = MDP_DISPLAY_COMMIT_OVERLAY;
    commit.wait_for_finish = 1;
    ret = ioctl(fbfd, MSMFB_DISPLAY_COMMIT, &commit);
    if (ret < 0) {
        printf("(%s) MSMFB_DISPLAY_COMMIT failed", __func__);
        return ret;
    }
    return overlay->id;
}
 
// clenaup array and unset overlay from hardware
int cleanup_overlay2(int id) {
    struct mdp_overlay *overlay;
    for (int i = 0; i < MAX_OVERLAYS; i++) {
        overlay = &overlays[i];
        //printf("(%s) check if overlays[%d].id == %d \n", __func__, i, id);
        if (overlay->id == id) {
            printf("(%s) overlays[%d].id=%d will be removed\n", __func__, i, overlay->id);
            unset_overlay2(id);
            overlay->id = -1;
            printf("(%s) overlays[%d].id = %d\n", __func__, i, overlay->id);
            break;
        }
    }

}

// unset overlay from hardware
int unset_overlay2(int id) {
    int ret = 0;
    struct mdp_display_commit commit = {0};
    
    // 1. Unset the overlay if it exists
//    printf("\n(%s): unset overlay.id %d\n", __func__, id);
    if (id != MSMFB_NEW_REQUEST) {
        ret = ioctl(fbfd, MSMFB_OVERLAY_UNSET, &id);
        if (ret < 0) {
            printf("MSMFB_OVERLAY_UNSET failed");
            // Don't return here - we still want to try committing
        }
//        printf("(%s): result: %d\n",__func__, ret);
        printf("(%s): overlay.id %d unset!\n",__func__, id);
    }
    
    // 2. Commit the change to the display
    commit.flags = MDP_DISPLAY_COMMIT_OVERLAY;
    commit.wait_for_finish = 1;  // Wait for the hardware to finish
    
    ret = ioctl(fbfd, MSMFB_DISPLAY_COMMIT, &commit);
    if (ret < 0) {
        printf("MSMFB_DISPLAY_COMMIT (cleanup) failed");
        return -1;
    }
}

// load raw data image. width and height has to be put manualy. 
int __place_file_to_ion(struct memPicture *mem_info) {
    printf("(%s): filename: %s\n",__func__, mem_info->filename);
    size_t size;
    GGLubyte* data = (GGLubyte*)read_file_mmap(mem_info->filename, &size);
    if (data) {
        mem_info->size = size;
        alloc_ION_mem(mem_info);
        memcpy(mem_info->mem_buf, data, size);
    }
    print_memory_info(mem_info);
}

int place_file_to_ion(struct memPicture *mem_info) {
    printf("(%s): filename: %s\n",__func__, mem_info->filename);
    GGLubyte* data = (GGLubyte*)get_bitmap(mem_info);
    if (data) {
        alloc_ION_mem(mem_info);
        memcpy(mem_info->mem_buf, data, mem_info->size);
        print_memory_info(mem_info);
    }
}

int load_file_to_ion(struct memPicture *mem_info, const char *filename) {
    printf("(%s) ----- struct pointer --> %p \n", __func__, (void *)mem_info);
    printf("(%s) filename: %s\n",__func__, filename);
    //printf("(%s) mem_info->name size: %d\n", __func__, sizeof(mem_info->name));
    //printf("(%s) mem_info->name pointer: %p\n", __func__, (void *)mem_info->name);
    snprintf(mem_info->name, sizeof(mem_info->name), "%s", filename);
    //printf("(%s) mem_info->name: %s\n",__func__, mem_info->name);
    GGLubyte* data = (GGLubyte*)get_bitmap(mem_info);
    if (data) {
        alloc_ION_mem(mem_info);
        memcpy(mem_info->mem_buf, data, mem_info->size);
        print_memory_info(mem_info);
    }
    printf("(%s) END\n",__func__);
}


int add_bitmap_to_ion(const char *name) {
    int slot = -1;
    for (int i = 0; i <= MAX_PICTURES; i++) {
        struct memPicture *pic = &pictures[i];
        printf("\n(%s) Check if pictures[%d].mem_fd == 0\n", __func__, i);
        //printf("(%s) pictures[%d] memory info \n", __func__, i);
        //print_memory_info(pic);
        if (pic->mem_fd == 0) {
            printf("(%s) slot %d is empty \n", __func__, i);
            load_file_to_ion(pic, name);
            slot = i;
            break;

        }
    }
    return slot;
}

// clear given slot of pictures array 
int cleanup_pictures(int id) {
    printf("(%s) pictures[%d] will be cleared\n",__func__, id);
    struct memPicture *pic = &pictures[id];
    free_ION_mem(pic);
    //pic->mem_fd = -1;
    printf("(%s) END\n",__func__);
}



//make_new_blit_list(int count) {
//    size_t sz = sizeof(struct mdp_blit_req_list) + count * sizeof(struct mdp_blit_req);
//    struct mdp_blit_req_list *blit_list = malloc(sz);
//    blit_list->count = 0;
//}

// temproraru use for blit test
void clear_half() {
        struct memPicture *image = &pictures[1];
        memset(image->mem_buf, 255, image->size/2);
}

void blit_now() {
    printf("(%s) MSMFB_BLIT Now\n",__func__);
    printf("(%s) blit.count: %d\n",__func__, blit_list.count);
    blit_list.count = 1; //--;
    printf("(%s) blit.count: %d\n",__func__, blit_list.count);
    int ret = 0;
    ret = ioctl(fbfd, MSMFB_BLIT, &blit_list);
    if (ret < 0) {
        printf("(%s) MSMFB_BLIT Failed\n",__func__);
    }
    printf("(%s) MSMFB_BLIT res: %d \n",__func__,ret);
    //exit(0);
}


add_to_blit_list(const char *type, const struct element *e) {
    printf("(%s) ----- struct pointer --> %p \n", __func__, (void *)e);
    printf("(%s) elem->label = %s \n", __func__, e->label);
    printf("(%s) Set into blit_list[%d] \n", __func__, blit_list.count);
        struct {
            uint32_t count;
            struct mdp_blit_req req[12];
        } list;
    if (blit_list.count < MAX_BLITS) {
        memset(&list, 0, sizeof(list));
        list.count = 1;
    
        struct mdp_blit_req *req  = &list.req[0];

        struct memPicture *atlas = &pictures[0];  // here is ui_atlas.png or theme.png
        req->src.width = atlas->width;
        req->src.height = atlas->height;
        req->src.offset = 0;
        req->src.format = MDP_RGBA_8888;
        req->src.memory_id = atlas->mem_fd;
        //req->src.memory_id = fbfd;

        struct memPicture *image = &pictures[0];
        req->dst.width = 1080; //image->width;
        req->dst.height = 1920; //image->height;
        req->dst.offset = 0;
        req->dst.format = MDP_RGBA_8888;
        req->dst.memory_id = image->mem_fd;
        req->dst.memory_id = fbfd;
        
        
        req->src_rect.x = 900;
        req->src_rect.y = 200;
        req->src_rect.w = 100;
        req->src_rect.h = 100;

        req->dst_rect.x = 900;
        req->dst_rect.y = 200;
        req->dst_rect.w = 100;
        req->dst_rect.h = 100;

    req->alpha       = 0xFF;             // Fully opaque 
    req->flags       = MDP_MEMORY_ID_TYPE_FB;
    req->transp_mask = MDP_TRANSP_NOP;
    req->transp_mask = 0xFF;
    print_blit_req(req);
    ioctl(fbfd, MSMFB_BLIT, &list);
    sleep(3);
    printf("(%s) exit \n",__func__);
exit(0);
//blit_now();
        blit_list.count++;
//blit_now();
//exit(0);
       
    }

}

