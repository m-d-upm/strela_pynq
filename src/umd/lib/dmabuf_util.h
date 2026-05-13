#ifndef DMABUF_UTIL_H
#define DMABUF_UTIL_H

#include <stddef.h>

int dmabuf_open();
void dmabuf_close(int fd);
int dmabuf_alloc(int fd, size_t size);
void* dmabuf_mmap(int fd, size_t size);
void dmabuf_unmap(void* ptr, size_t size);
void dmabuf_free(int fd);
int dmabuf_sync_start(int fd);
int dmabuf_sync_end(int fd);

#endif // DMABUF_UTIL_H
