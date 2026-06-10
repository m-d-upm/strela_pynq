#include "accel_lib.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>  // mmap()
#include <stdint.h>

#include <time.h>

#include "utilities.h"

#include "buffer_cacheable_test.h"

#define TRANSFER_SIZE (8192) // 32 KB
#define ITERATIONS    (100000)

static int indices[TRANSFER_SIZE];

static void shuffle_indices(int *array, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

static void buffer_test(int *buffer)
{
    volatile int dummy = 0;

    for (int i = 0; i < TRANSFER_SIZE; i++)
    {
        buffer[i] = i % 2 ? i : -i;
        indices[i] = i;
    }

    srand(time(NULL));

    // sequential
    uint64_t start_seq = micros();

    for (int iterations = 0; iterations < ITERATIONS; iterations++)
    {
        for (int i = 0; i < TRANSFER_SIZE; i++)
            dummy = buffer[i];
    }

    uint64_t end_seq = micros();

    uint64_t time_seq_micros = end_seq - start_seq;

    // random
    shuffle_indices(indices, TRANSFER_SIZE);

    uint64_t start_rand = micros();

    for (int iterations = 0; iterations < ITERATIONS; iterations++)
    {
        for (int i = 0; i < TRANSFER_SIZE; i++)
            dummy = buffer[indices[i]];
    }

    uint64_t end_rand = micros();

    uint64_t time_rand_micros = end_rand - start_rand;

    printf("Results (%u elements - %u KB - iterations %u)\n", TRANSFER_SIZE, (TRANSFER_SIZE * sizeof(int)) / 1024, ITERATIONS);
    printf("Sequential access time: %llu microseconds\n", time_seq_micros);
    printf("Random access time: %llu microseconds\n", time_rand_micros);
}

void buffer_cacheable_test()
{
    int *buffer_malloc = malloc(TRANSFER_SIZE * sizeof(int));

    if(!buffer_malloc)
    {
        printf("Error when trying to allocate with malloc!\n");
        goto error_malloc;
    }

    printf("Running malloc buffer test...\n");

    buffer_test(buffer_malloc);

    int file_desc_buffer_dmabuf = accel_lib_buf_alloc(TRANSFER_SIZE * sizeof(int));

    if (file_desc_buffer_dmabuf < 0) {
        printf("Can't allocate dmabuf buffer!\n");

        goto error_alloc_buf;
    }

    int *buffer_dmabuf = (int32_t*) accel_lib_buf_map(file_desc_buffer_dmabuf, TRANSFER_SIZE * sizeof(int));

    if(!buffer_dmabuf)
    {
        printf("Can't mmap dmabuf buffer!\n");
        goto error_mmap;
    }

    printf("Running dmabuf buffer test...\n");

    buffer_test(buffer_dmabuf);

    accel_lib_buf_unmap(buffer_dmabuf, TRANSFER_SIZE * sizeof(int));
    accel_lib_buf_dealloc(file_desc_buffer_dmabuf);
    free(buffer_malloc);

    return;

error_mmap:
    accel_lib_buf_dealloc(file_desc_buffer_dmabuf);
error_alloc_buf:
    free(buffer_malloc);
error_malloc:
}
