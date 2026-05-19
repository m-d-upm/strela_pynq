#include "dmabuf_util.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#include <unistd.h>

int dmabuf_open()
{
    int fd;

    // requires a CMA region defined in the device tree
    fd = open("/dev/dma_heap/reserved", O_RDWR, 0);

    return fd; // fd of the dma-buf heap framework object
}

void dmabuf_close(int fd)
{
    close(fd); // fd of the dma-buf heap framework object
}

int dmabuf_alloc(int fd, size_t size)
{
  struct dma_heap_allocation_data dmabuf_alloc = { 0 };

  dmabuf_alloc.len = size; // in bytes
  dmabuf_alloc.fd_flags = O_CLOEXEC | O_RDWR;

  ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &dmabuf_alloc);

  return dmabuf_alloc.fd; // fd of the buffer allocated from dma-buf heap framework
}

void* dmabuf_mmap(int fd, size_t size)
{
  void *ptr = NULL;

  ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // fd of the buffer allocated from dma-buf heap framework

  return ptr;
}

void dmabuf_unmap(void* ptr, size_t size)
{
    munmap(ptr, size);
}

void dmabuf_free(int fd)
{
    close(fd); // fd of the buffer allocated from dma-buf heap framework
}

int dmabuf_sync_start(int fd)
{
    struct dma_buf_sync sync_start = { 0 };

    sync_start.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;

    if(ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync_start) == 0) // fd of the buffer allocated from dma-buf heap framework
        return 0;

    return -1;
}

int dmabuf_sync_end(int fd)
{
    struct dma_buf_sync sync_end = { 0 };

    sync_end.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;

    if(ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync_end) == 0) // fd of the buffer allocated from dma-buf heap framework
        return 0;

    return -1;
}
