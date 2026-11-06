#ifndef IZUMI_TEST_H
#define IZUMI_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unlike assert(), checks must still run when built with -DNDEBUG. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define RUN(test) do { test(); puts("PASS: " #test); } while (0)

#endif
