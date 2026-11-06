#ifndef IZUMI_ALLOCATION_H
#define IZUMI_ALLOCATION_H

#include <stdio.h>
#include <stdlib.h>

static inline void *izumi_realloc(void *pointer, size_t size) {
    void *result = realloc(pointer, size);
    if (result == NULL) {
        fprintf(stderr, "Error: Could not allocate %zu bytes\n", size);
        exit(1);
    }
    return result;
}

static inline void *izumi_malloc(size_t size) {
    return izumi_realloc(NULL, size);
}

#endif
