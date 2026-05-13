#include "accel_lib.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#include <unistd.h>

int accel_lib_buf_alloc(size_t size)
{
    int dmabuf_heap_fd = -1;
    int buf_fd = -1;

    if (size == 0)
        return -1;

    dmabuf_heap_fd = dmabuf_open();

    if(dmabuf_heap_fd < 0)
        return -1;

    buf_fd = dmabuf_alloc(dmabuf_heap_fd, size);

    dmabuf_close(dmabuf_heap_fd);
    
    return buf_fd;
}

void* accel_lib_buf_map(int buf_fd, size_t size)
{
    int dmabuf_heap_fd = -1;
    void *ptr = NULL;

    if (buf_fd < 0)
        return NULL;

    dmabuf_heap_fd = dmabuf_open();

    if(dmabuf_heap_fd < 0)
        return NULL;

    ptr = dmabuf_mmap(buf_fd, size);

    dmabuf_close(dmabuf_heap_fd);

    return ptr;
}

void accel_lib_buf_unmap(void *buf_ptr, size_t size)
{
    int dmabuf_heap_fd = -1;
    void *ptr = NULL;

    if (!buf_ptr)
        return;

    dmabuf_heap_fd = dmabuf_open();

    if(dmabuf_heap_fd < 0)
        return;

    dmabuf_unmap(buf_ptr, size);

    dmabuf_close(dmabuf_heap_fd);
}

void accel_lib_buf_dealloc(int buf_fd)
{
    int dmabuf_heap_fd = -1;

    if (buf_fd < 0)
        return;

    dmabuf_heap_fd = dmabuf_open();

    if(dmabuf_heap_fd < 0)
        return;

    dmabuf_free(buf_fd);

    dmabuf_close(dmabuf_heap_fd);
}

int accel_lib_attach_buf_to_dev(struct accel_attach_info *info)
{
    if (!info)
        return -1;
        
    if(info->accel_fd < 0)
        return -1;
    
    if (info->buf_fd < 0)
        return -1;
      
    if (!info->accel_buf_attach)
        return -1; 
        
    if(info->accel_buf_attach(info->accel_fd, info->buf_fd , info->direction) < 0)
        return -1;

    return 0;
}

int accel_lib_detach_buf_from_dev(struct accel_attach_info *info)
{
      if (!info)
        return -1;
        
    if(info->accel_fd < 0)
        return -1;
    
    if (info->buf_fd < 0)
        return -1;
      
    if (!info->accel_buf_detach)
        return -1; 
        
    if(info->accel_buf_detach(info->accel_fd, info->direction) < 0)
        return -1;

    return 0;
}
