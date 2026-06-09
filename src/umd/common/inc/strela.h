/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2025 Juan Granja, CEI-UPM.
 * Copyright (C) 2025 Milos Dordevic, CEI-UPM.
 */

#ifndef _STRELA_H_
#define _STRELA_H_

#include <stdint.h>

#define BASE_DEVICE_PATH ("/dev/strela")

struct strela_csrs {
    uint32_t b;
    uint32_t a;

    uint32_t conf_offs;
    uint32_t conf_count;

    uint32_t in0_offs;
    uint32_t in0_count;
    uint32_t in0_stride;
    uint32_t in1_offs;
    uint32_t in1_count;
    uint32_t in1_stride;
    uint32_t in2_offs;
    uint32_t in2_count;
    uint32_t in2_stride;
    uint32_t in3_offs;
    uint32_t in3_count;
    uint32_t in3_stride;

    uint32_t out0_offs;
    uint32_t out0_count;
    uint32_t out1_offs;
    uint32_t out1_count;
    uint32_t out2_offs;
    uint32_t out2_count;
    uint32_t out3_offs;
    uint32_t out3_count;
};

struct strela_ctrl {
    struct strela_csrs csrs;
};

#define IOCTL_BASE 'W'

#define IOCTL_STRELA_CONTROL _IOW(IOCTL_BASE, 1, struct strela_ctrl)
#define IOCTL_STRELA_CONFIG  _IO(IOCTL_BASE, 2)
#define IOCTL_STRELA_EXEC    _IO(IOCTL_BASE, 3)

#define IOCTL_STRELA_ATTACH_IN_BUF  _IOW(IOCTL_BASE, 4, int)
#define IOCTL_STRELA_ATTACH_OUT_BUF _IOW(IOCTL_BASE, 5, int)
#define IOCTL_STRELA_DETACH_IN_BUF  _IO(IOCTL_BASE, 6)
#define IOCTL_STRELA_DETACH_OUT_BUF _IO(IOCTL_BASE, 7)

#endif

