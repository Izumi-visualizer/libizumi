#ifndef IZUMI__VISUALIZER__VISUALIZER_H
#define IZUMI__VISUALIZER__VISUALIZER_H

#include <stdbool.h>

#include "instruction_table_array.h"

typedef void * izumi_handle_t;

typedef enum {
    IZUMI_ERROR_KIND__OK = 0,
    IZUMI_ERROR_KIND__IO,
    IZUMI_ERROR_KIND__PARSER,
    IZUMI_ERROR_KIND__UNKNOWN,
} izumi_error_kind_t;

typedef struct {
    union {
        izumi_error_kind_t kind;
        bool present;
    };
    char * details;
} izumi_error_t;

izumi_handle_t izumi_prepare(const char * trace_path);

izumi_error_t izumi_parse(izumi_handle_t handle, size_t length_hint, izumi_instruction_table_array_t * ita);

#endif