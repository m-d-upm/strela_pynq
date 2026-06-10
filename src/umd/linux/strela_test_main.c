// Copyright 2025 CEI - UPM.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Juan Granja <juan.granja@upm.es>
// Milos Dordevic <milos.dordevic@upm.es>

#include <stdio.h>

#include <pthread.h>

#include "bypass.h"

#include "relu.h"

#include "buffer_cacheable_test.h"

#include "mat_mul.h"

int main(int argc, char* argv[])
{
    //pthread_t bypass_test_th;

    //printf("Running bypass test...\r\n");
    //bypass_test();

    //printf("Running ReLu test...\r\n");
    //relu_test();

    printf("Running matrix multiplication test...\r\n");
    mat_mul_test();

    //printf("Running a test to see if dmabuf(s) are cacheable...\r\n");
    //buffer_cacheable_test();

    //pthread_create(&bypass_test_th, NULL, bypass_test, NULL);
    
    //pthread_join(bypass_test_th, NULL);

    return 0;
}
