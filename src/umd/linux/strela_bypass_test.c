// Copyright 2025 CEI - UPM.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Juan Granja <juan.granja@upm.es>
// Milos Dordevic <milos.dordevic@upm.es>

#include "strela.h" 
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

#define DEV_NAME "/dev/strela0"

//#define TRANSFER_SIZE (8192) // 32 KB
 #define TRANSFER_SIZE (20) // B

int32_t input_data_sw[TRANSFER_SIZE];
int32_t output_data_sw[TRANSFER_SIZE];

#define BYPASS_KRNL_NPE (16)
#define BYPASS_KRNL_SIZE (BYPASS_KRNL_NPE * 5)
#define BYPASS_KRNL_BYTES (BYPASS_KRNL_SIZE * sizeof(uint32_t))

uint32_t bypass_kernel[BYPASS_KRNL_SIZE] = {
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 12
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 8
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 4
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0

    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 13
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 9
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 5
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 1

    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 14
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 10
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 6
    0x00000021, 0x00000000, 0x00000012, 0x00000000, 0x00000000, // 2

    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 15
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 11
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 7
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000  // 3
};

static int strela_attach(int accel_fd, int buf_fd, enum accel_shbuf_dir direction)
{
    long cmd;
    
    cmd = direction == ACCEL_SHBUF_DIR_IN ? IOCTL_STRELA_ATTACH_IN_BUF : IOCTL_STRELA_ATTACH_OUT_BUF;

    if (ioctl(accel_fd, cmd, &buf_fd) != 0)
    {
        printf("ERROR: Couldn't attach buffer to the device!\n");
        return -1;
    }

    return 0;
}

static int strela_detach(int accel_fd, enum accel_shbuf_dir direction)
{
    long cmd;
    
    cmd = direction == ACCEL_SHBUF_DIR_IN ? IOCTL_STRELA_DETACH_IN_BUF : IOCTL_STRELA_DETACH_OUT_BUF;

    if (ioctl(accel_fd, cmd) != 0)
    {
        printf("ERROR: Couldn't detach buffer from the device!\n");
        return -1;
    }

    return 0;
}

void bypass_test()
{
    int32_t *input = NULL;
    int32_t *result = NULL;
    int32_t *conf = NULL;

    int file_desc_strela;

    int file_desc_buf_in;
    int file_desc_buf_out;
    int file_desc_buf_conf;

    struct accel_attach_info strela_attach_info;
    
    file_desc_strela = open(DEV_NAME, O_RDWR);

    if (file_desc_strela < 0) {
        printf("Can't open device file: %s, error:%d\n", DEV_NAME, file_desc_strela);
        goto error;
    }

    printf("\n---------\n");

    file_desc_buf_in = accel_lib_buf_alloc(TRANSFER_SIZE * sizeof(int32_t));

    if (file_desc_buf_in < 0) {
        printf("Can't allocate input dmabuf\n");

        goto error_alloc_buf_in;
    }


    file_desc_buf_out = accel_lib_buf_alloc(TRANSFER_SIZE * sizeof(int32_t));

    if (file_desc_buf_out < 0) {
        printf("Can't allocate output dmabuf\n");

        goto error_alloc_buf_out;
    }


    file_desc_buf_conf = accel_lib_buf_alloc(BYPASS_KRNL_BYTES);

    if (file_desc_buf_conf < 0) {
        printf("Can't allocate config dmabuf\n");

        goto error_alloc_buf_conf;
    }

    input = (int32_t*) accel_lib_buf_map(file_desc_buf_in, TRANSFER_SIZE * sizeof(int32_t));

    if(!input)
    {
        goto error_mmap_in;
    }

    result = (int32_t*) accel_lib_buf_map(file_desc_buf_out, TRANSFER_SIZE * sizeof(int32_t));

    if(!result)
    {
        goto error_mmap_out;
    }

    conf = (int32_t*) accel_lib_buf_map(file_desc_buf_conf, BYPASS_KRNL_BYTES);

    if(!conf)
    {
        goto error_mmap_conf;
    }

    // Populate input data
    for(int i = 0; i < TRANSFER_SIZE; i++)
    {
        input[i] = i % 2 ? i : -i;
    }

    for(int i = 0; i < TRANSFER_SIZE; i++)
    {
        input_data_sw[i] = i % 2 ? i : -i;
    }

    // Read input data befor write (test cache flushing)
    printf("OUTPUT before (first twenty 32-bit elements):\n");

    dmabuf_sync_start(file_desc_buf_out);

    for(int i = 0; i < 20; i++)
    {
        result[i] = 0xffffffff;
    }

    dmabuf_sync_end(file_desc_buf_out);

    examine_mem(result, 0, 20);

    // Copy config to buffer
    uint32_t *cgra_kernel = bypass_kernel;
    uint32_t cgra_kernel_size_words = BYPASS_KRNL_SIZE;

    printf("Copying config...\n");

    uint64_t begin_write_config = micros();

    dmabuf_sync_start(file_desc_buf_conf);

    memcpy(conf, cgra_kernel, cgra_kernel_size_words * sizeof(uint32_t));

    dmabuf_sync_end(file_desc_buf_conf);

    uint64_t end_write_config = micros();

    printf("Setting up config transfer...\n");

    uint64_t begin_cfg_setup_transf = micros();

    struct strela_ctrl cgra_ctrl = {0};

    strela_attach_info.accel_buf_attach = strela_attach;
    strela_attach_info.accel_buf_detach = strela_detach;
    strela_attach_info.accel_fd = file_desc_strela;
    
    strela_attach_info.direction = ACCEL_SHBUF_DIR_IN;
    strela_attach_info.buf_fd = file_desc_buf_conf;

    if (accel_lib_attach_buf_to_dev(&strela_attach_info) != 0)
        goto error_attach_buf_conf;
    
    cgra_ctrl.csrs.conf_offs = 0;
    cgra_ctrl.csrs.conf_count = cgra_kernel_size_words;

    if (ioctl(file_desc_strela, IOCTL_STRELA_CONTROL, &cgra_ctrl) != 0)
    {
        printf("ERROR: Setting up config transfer!\n");
        goto error_strela_ioctl_conf;
    }

    uint64_t end_cfg_setup_transf = micros();

    // Configure 1

    printf("Transfering config to the device...\n");

    uint64_t begin_cgra_config = micros();

    if (ioctl(file_desc_strela, IOCTL_STRELA_CONFIG) != 0)
    {
        printf("ERROR: Transfering config to the device!\n");
        goto error_strela_ioctl_conf;
    }

    uint64_t end_cgra_config = micros();

    printf("Setting up transfer...\n");

    uint64_t begin_setup_transf = micros();

    accel_lib_detach_buf_from_dev(&strela_attach_info);

    strela_attach_info.direction = ACCEL_SHBUF_DIR_IN;
    strela_attach_info.buf_fd = file_desc_buf_in;

    if (accel_lib_attach_buf_to_dev(&strela_attach_info) != 0)
        goto error_attach_buf_in;

    strela_attach_info.direction = ACCEL_SHBUF_DIR_OUT;
    strela_attach_info.buf_fd = file_desc_buf_out;

    if (accel_lib_attach_buf_to_dev(&strela_attach_info) != 0)
        goto error_attach_buf_out;

    cgra_ctrl.csrs.conf_offs = 0;
    cgra_ctrl.csrs.conf_count = 0;

    cgra_ctrl.csrs.in0_offs = 0;
    cgra_ctrl.csrs.in0_count = TRANSFER_SIZE;
    cgra_ctrl.csrs.in0_stride = 4;

    cgra_ctrl.csrs.out0_offs = 0;
    cgra_ctrl.csrs.out0_count = TRANSFER_SIZE;

    if (ioctl(file_desc_strela, IOCTL_STRELA_CONTROL, &cgra_ctrl) != 0)
    {
        printf("ERROR: Setting up transfer!\n");
        goto error_strela_ioctl;
    }

    uint64_t end_setup_transf = micros();

    // Execute
    printf("Executing...\n");

    uint64_t begin_cgra_exec = micros();

    if (ioctl(file_desc_strela, IOCTL_STRELA_EXEC) != 0)
    {
        printf("ERROR: Timeout while executing!\n");
        goto error_strela_ioctl;
    }

    uint64_t end_cgra_exec = micros();

    printf("Running pure software implementation (without using the accelerator)...\n");

    uint64_t begin_sw = micros();

    for(int i = 0; i< TRANSFER_SIZE; i++)
    {
        output_data_sw[i] = input_data_sw[i];
    }

    uint64_t end_sw = micros();

    printf("Input (first twenty 32-bit elements) -----------\n");
    examine_mem(input, 0, 20);

    dmabuf_sync_start(file_desc_buf_out);

    printf("Output CGRA (first twenty 32-bit elements) -----------\n");
    examine_mem(result, 0, 20);

    dmabuf_sync_end(file_desc_buf_out);

    printf("Output SW (CPU) (first twenty 32-bit elements) -----------\n");
    examine_mem(output_data_sw, 0, 20);

    unsigned total_cgra = 0;
    unsigned delta_cycles;

    delta_cycles = end_write_config - begin_write_config;
    printf("Write config (ms): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = end_cfg_setup_transf - begin_cfg_setup_transf;
    printf("Setup config transfer (ms): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = end_cgra_config - begin_cgra_config;
    printf("Config (ms): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = end_setup_transf - begin_setup_transf;
    printf("Setup transfer (ms): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = end_cgra_exec - begin_cgra_exec;
    printf("Execute (ms): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    printf("Total CGRA (ms): %u\n", total_cgra);

    delta_cycles = end_sw - begin_sw;
    printf("CPU (ms): %u\n", delta_cycles);

    uint64_t a, b;
    a = micros();
    b = micros();
    printf("Min (ms): %llu\n", b - a);

    strela_attach_info.direction = ACCEL_SHBUF_DIR_IN;
    strela_attach_info.buf_fd = file_desc_buf_in;

    accel_lib_detach_buf_from_dev(&strela_attach_info);
    
    strela_attach_info.direction = ACCEL_SHBUF_DIR_OUT;
    strela_attach_info.buf_fd = file_desc_buf_out;

    accel_lib_detach_buf_from_dev(&strela_attach_info);

    accel_lib_buf_unmap(file_desc_buf_out, TRANSFER_SIZE * sizeof(int32_t));
    accel_lib_buf_unmap(file_desc_buf_in, TRANSFER_SIZE * sizeof(int32_t));
    accel_lib_buf_unmap(file_desc_buf_conf, BYPASS_KRNL_BYTES);
    accel_lib_buf_dealloc(file_desc_buf_conf);
    accel_lib_buf_dealloc(file_desc_buf_in);
    accel_lib_buf_dealloc(file_desc_buf_out);

    close(file_desc_strela);
    
    return;

error_strela_ioctl:
    strela_attach_info.direction = ACCEL_SHBUF_DIR_OUT;
    strela_attach_info.buf_fd = file_desc_buf_out;

    accel_lib_detach_buf_from_dev(&strela_attach_info);
error_attach_buf_out:
    strela_attach_info.direction = ACCEL_SHBUF_DIR_IN;
    strela_attach_info.buf_fd = file_desc_buf_in;

    accel_lib_detach_buf_from_dev(&strela_attach_info);
error_attach_buf_in:
error_strela_ioctl_conf:
error_attach_buf_conf:
    accel_lib_buf_unmap(file_desc_buf_conf, BYPASS_KRNL_BYTES);
error_mmap_conf:
    accel_lib_buf_unmap(file_desc_buf_out, TRANSFER_SIZE * sizeof(int32_t));
error_mmap_out:
    accel_lib_buf_unmap(file_desc_buf_in, TRANSFER_SIZE * sizeof(int32_t));
error_mmap_in:
    accel_lib_buf_dealloc(file_desc_buf_conf);
error_alloc_buf_conf:
    accel_lib_buf_dealloc(file_desc_buf_out);
error_alloc_buf_out:
    accel_lib_buf_dealloc(file_desc_buf_in);
error_alloc_buf_in:
    close(file_desc_strela);
error:
    exit(EXIT_FAILURE);
}
