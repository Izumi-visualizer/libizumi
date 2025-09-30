#ifndef IZUMI__UTIL__VECTOR_H
#define IZUMI__UTIL__VECTOR_H

#include <stddef.h>

typedef struct {
    void * data;
    size_t capacity;
    size_t length;
} izumi_vector_t;

void izumi_vector__init(izumi_vector_t * vec);
void izumi_vector__destroy(izumi_vector_t * vec);

void * izumi_vector__extend(izumi_vector_t * vec, size_t elem_size, size_t count);
void * izumi_vector__push(izumi_vector_t * vec, void * elem);

void * izumi_vector__get(izumi_vector_t * vec, size_t elem_size, size_t idx);

#endif
