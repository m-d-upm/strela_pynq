// Copyright 2025 CEI - UPM.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
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

#include "mat_mul.h"

typedef int32_t strela_data_t;

#define DEV_NAME "/dev/strela0"

#define EXAMINE_MEM_ELEMENTS (40)

#define MATRIX_DIM (10) // 10 words (either 32-bit or 64-bit depends on the CGRA)
#define MATRIX_SIZE (MATRIX_DIM * MATRIX_DIM) // square matrix

static strela_data_t input_data_matrix_sw_A[MATRIX_SIZE];
static strela_data_t input_data_matrix_sw_B[MATRIX_SIZE];
static strela_data_t output_data_matrix_sw[MATRIX_SIZE];

#define MAT_MUL_KRNL_NPE (16)
#define MAT_MUL_KRNL_SIZE (MAT_MUL_KRNL_NPE * 5)
#define MAT_MUL_KRNL_BYTES (MAT_MUL_KRNL_SIZE * sizeof(uint32_t))

static uint32_t mat_mul_kernel[MAT_MUL_KRNL_SIZE] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 12
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 4
    0x00000041, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0

    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 13
    0x00000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 9
    0x00000201, 0xC0040400, 0x000A0080, 0x00000000, 0x00000000, // 5
    0x04000209, 0x018C0300, 0x00000082, 0x00000000, 0x00000000, // 1

    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 14
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 10
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 6
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 2

    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 15
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 11
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 7
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 // 3
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

void mat_mul_test()
{
    strela_data_t *input = NULL;
    strela_data_t *result = NULL;
    strela_data_t *conf = NULL;

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

    printf("Product of two matrices (%u x %u) ---------\n", MATRIX_DIM, MATRIX_DIM);

    file_desc_buf_in = accel_lib_buf_alloc(MATRIX_SIZE * sizeof(strela_data_t) * 2);

    if (file_desc_buf_in < 0) {
        printf("Can't allocate input dmabuf\n");

        goto error_alloc_buf_in;
    }

    file_desc_buf_out = accel_lib_buf_alloc(MATRIX_SIZE * sizeof(strela_data_t));

    if (file_desc_buf_out < 0) {
        printf("Can't allocate output matrix dmabuf\n");

        goto error_alloc_buf_out;
    }

    file_desc_buf_conf = accel_lib_buf_alloc(MAT_MUL_KRNL_BYTES);

    if (file_desc_buf_conf < 0) {
        printf("Can't allocate config dmabuf\n");

        goto error_alloc_buf_conf;
    }

    input = (strela_data_t*) accel_lib_buf_map(file_desc_buf_in, MATRIX_SIZE * sizeof(strela_data_t) * 2);

    if(!input)
    {
        goto error_mmap_in;
    }

    result = (strela_data_t*) accel_lib_buf_map(file_desc_buf_out, MATRIX_SIZE * sizeof(strela_data_t));

    if(!result)
    {
        goto error_mmap_out;
    }

    conf = (strela_data_t*) accel_lib_buf_map(file_desc_buf_conf, MAT_MUL_KRNL_BYTES);

    if(!conf)
    {
        goto error_mmap_conf;
    }

    dmabuf_sync_start(file_desc_buf_in);

    // Populate input data
    for(int i = 0; i < MATRIX_SIZE; i++)
    {
        input[i] = i % 2 ? i : -i;
    }

    for(int i = MATRIX_SIZE; i < MATRIX_SIZE * 2; i++)
    {
        input[i] = i % 2 ? i : -i;
    }

    dmabuf_sync_end(file_desc_buf_in);

    for(int i = 0; i < MATRIX_SIZE; i++)
    {
        input_data_matrix_sw_A[i] = i % 2 ? i : -i;
        input_data_matrix_sw_B[i] = (i + MATRIX_SIZE) % 2 ? (i + MATRIX_SIZE) : -(i + MATRIX_SIZE);
    }

    // Read input data before write (test cache flushing)
    printf("OUTPUT before (first %d 32-bit elements):\n", EXAMINE_MEM_ELEMENTS);

    dmabuf_sync_start(file_desc_buf_out);

    for(int i = 0; i < 20; i++)
    {
        result[i] = 0xffffffff;
    }

    examine_mem(result, 0, EXAMINE_MEM_ELEMENTS);

    dmabuf_sync_end(file_desc_buf_out);

    // Copy config to buffer
    uint32_t *cgra_kernel = mat_mul_kernel;
    uint32_t cgra_kernel_size_bytes = MAT_MUL_KRNL_BYTES;

    printf("Copying config...\n");

    uint64_t begin_write_config = micros();

    dmabuf_sync_start(file_desc_buf_conf);

    memcpy(conf, cgra_kernel, cgra_kernel_size_bytes);

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
    cgra_ctrl.csrs.conf_count = cgra_kernel_size_bytes;

    if (ioctl(file_desc_strela, IOCTL_STRELA_CONTROL, &cgra_ctrl) != 0)
    {
        printf("ERROR: Setting up config transfer!\n");
        goto error_strela_ioctl_conf;
    }

    uint64_t end_cfg_setup_transf = micros();

    // Configure

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

    uint64_t end_setup_transf = micros();

    uint64_t total_cgra_exec = 0;
    uint64_t total_setup_transf = end_setup_transf - begin_setup_transf;

    printf("Executing...\n");

    for (int i = 0; i < MATRIX_DIM; i++) // process the rest of the matrices
        for (int j = 0; j < MATRIX_DIM; j++)
            {
                uint64_t begin_setup_transf_loop = micros();

                cgra_ctrl.csrs.in0_offs = i * MATRIX_DIM * sizeof(strela_data_t);
                cgra_ctrl.csrs.in0_count = MATRIX_DIM * sizeof(strela_data_t);
                cgra_ctrl.csrs.in0_stride = sizeof(strela_data_t);

                cgra_ctrl.csrs.in1_offs = MATRIX_SIZE  * sizeof(strela_data_t) + j * MATRIX_DIM * sizeof(strela_data_t);
                cgra_ctrl.csrs.in1_count = MATRIX_DIM * sizeof(strela_data_t);
                cgra_ctrl.csrs.in1_stride = sizeof(strela_data_t);

                cgra_ctrl.csrs.out1_offs = (i * MATRIX_DIM + j) * sizeof(strela_data_t);
                cgra_ctrl.csrs.out1_count = sizeof(strela_data_t);

                if (ioctl(file_desc_strela, IOCTL_STRELA_CONTROL, &cgra_ctrl) != 0)
                {
                    printf("ERROR: Setting up transfer!\n");
                    goto error_strela_ioctl;
                }

                uint64_t end_setup_transf_loop = micros();

                uint64_t begin_cgra_exec_loop = micros();

                if (ioctl(file_desc_strela, IOCTL_STRELA_EXEC) != 0)
                {
                    printf("ERROR: Timeout while executing!\n");
                    goto error_strela_ioctl;
                }

                uint64_t end_cgra_exec_loop = micros();

                total_cgra_exec += end_cgra_exec_loop - begin_cgra_exec_loop;
                total_setup_transf += end_setup_transf_loop - begin_setup_transf_loop;
        }

    printf("Running pure software implementation (without using the accelerator)...\n");

    int32_t *A = input_data_matrix_sw_A;
    int32_t *B = input_data_matrix_sw_B;
    int32_t *C = output_data_matrix_sw;

    uint64_t begin_sw = micros();

    for (int i = 0; i < MATRIX_DIM; i++)
        for (int j = 0; j < MATRIX_DIM; j++)
        {   
            int32_t sum = 0;

            for (int k = 0; k < MATRIX_DIM; k++)
                sum += *(A + i * MATRIX_DIM + k) * *(B + j * MATRIX_DIM + k);

            *(C + i * MATRIX_DIM + j) = sum;
        }
    
    uint64_t end_sw = micros();

    dmabuf_sync_start(file_desc_buf_in);

    printf("Input (first %d 32-bit elements) -----------\n", EXAMINE_MEM_ELEMENTS);
    examine_mem(input, 0, EXAMINE_MEM_ELEMENTS);

    dmabuf_sync_end(file_desc_buf_in);

    dmabuf_sync_start(file_desc_buf_out);

    printf("Output CGRA (first %d 32-bit elements) -----------\n", EXAMINE_MEM_ELEMENTS);
    examine_mem(result, 0, EXAMINE_MEM_ELEMENTS);

    dmabuf_sync_end(file_desc_buf_out);

    printf("Output SW (CPU) (first %d 32-bit elements) -----------\n", EXAMINE_MEM_ELEMENTS);
    examine_mem(output_data_matrix_sw, 0, EXAMINE_MEM_ELEMENTS);

    unsigned total_cgra = 0;
    unsigned delta_cycles;

    delta_cycles = end_write_config - begin_write_config;
    printf("Write config (us): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = end_cfg_setup_transf - begin_cfg_setup_transf;
    printf("Setup config transfer (us): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = end_cgra_config - begin_cgra_config;
    printf("Config (us): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = total_setup_transf;
    printf("Setup transfer (us): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    delta_cycles = total_cgra_exec;
    printf("Execute (us): %u\n", delta_cycles);
    total_cgra += delta_cycles;

    printf("Total CGRA (us): %u\n", total_cgra);

    delta_cycles = end_sw - begin_sw;
    printf("CPU (us): %u\n", delta_cycles);

    uint64_t a, b;
    a = micros();
    b = micros();
    printf("Min (us): %llu\n", b - a);

    validate_buffers(result, output_data_matrix_sw, MATRIX_SIZE * sizeof(strela_data_t));

    strela_attach_info.direction = ACCEL_SHBUF_DIR_IN;
    strela_attach_info.buf_fd = file_desc_buf_in;

    accel_lib_detach_buf_from_dev(&strela_attach_info);
    
    strela_attach_info.direction = ACCEL_SHBUF_DIR_OUT;
    strela_attach_info.buf_fd = file_desc_buf_out;

    accel_lib_detach_buf_from_dev(&strela_attach_info);

    accel_lib_buf_unmap(result, MATRIX_SIZE * sizeof(strela_data_t));
    accel_lib_buf_unmap(input, MATRIX_SIZE * sizeof(strela_data_t)  * 2);
    accel_lib_buf_unmap(conf, MAT_MUL_KRNL_BYTES);
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
    accel_lib_buf_unmap(conf, MAT_MUL_KRNL_BYTES);
error_mmap_conf:
    accel_lib_buf_unmap(result, MATRIX_SIZE * sizeof(strela_data_t));
error_mmap_out:
    accel_lib_buf_unmap(input, MATRIX_SIZE * sizeof(strela_data_t) * 2);
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
