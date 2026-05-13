#ifndef ACCEL_LIB_H
#define ACCEL_LIB_H

#include <stddef.h>

#include "dmabuf_util.h"

enum accel_shbuf_dir {
    ACCEL_SHBUF_DIR_IN = 0,
    ACCEL_SHBUF_DIR_OUT
};

struct accel_attach_info {
    int buf_fd;
    int accel_fd;
    enum accel_shbuf_dir direction;
    int (*accel_buf_attach)(int, int, enum accel_shbuf_dir);
    int (*accel_buf_detach)(int, enum accel_shbuf_dir);
};

int accel_lib_buf_alloc(size_t size);
void* accel_lib_buf_map(int buf_fd, size_t size);
void accel_lib_buf_unmap(void *buf_ptr, size_t size);
void accel_lib_buf_dealloc(int buf_fd);
int accel_lib_attach_buf_to_dev(struct accel_attach_info *info);
int accel_lib_detach_buf_from_dev(struct accel_attach_info *info);

#endif // ACCEL_LIB_H
