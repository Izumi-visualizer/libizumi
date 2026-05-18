#include "izumi/util/vector.h"

#include <stdlib.h>
#include <string.h>

void izumi_vector__init(izumi_vector_t *vec) {
    vec->data = NULL;
    vec->capacity = 0;
    vec->length = 0;
}

void izumi_vector__destroy(izumi_vector_t *vec) {
    free(vec->data);
    vec->data = NULL;
    vec->capacity = 0;
    vec->length = 0;
}

void *izumi_vector__extend(izumi_vector_t *vec, size_t elem_size, size_t count) {
    if (vec->data == NULL) {
        size_t init_size = ((count >> 9) + 1) << 9; // múltiplo de 512
        vec->data = malloc(elem_size * init_size);
        if (vec->data == NULL) return NULL;
        vec->capacity = init_size;
        vec->length = 0;
    } 
    else if (vec->length + count > vec->capacity) {
        size_t new_capacity = vec->capacity;
        while (new_capacity < vec->length + count)
            new_capacity *= 2;

        void *new_data = realloc(vec->data, elem_size * new_capacity);
        if (new_data == NULL) return NULL;

        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    void *res = (char*)vec->data + elem_size * vec->length;
    vec->length += count;

    return res;
}

void *izumi_vector__push(izumi_vector_t *vec, size_t elem_size, void *elem) {
    void *res_addr = izumi_vector__extend(vec, elem_size, 1);
    if (res_addr == NULL) return NULL;

    memcpy(res_addr, elem, elem_size);
    return res_addr;
}

void *izumi_vector__get(izumi_vector_t *vec, size_t elem_size, size_t idx) {
    if (idx >= vec->length) return NULL;
    return (char*)vec->data + elem_size * idx;
}

