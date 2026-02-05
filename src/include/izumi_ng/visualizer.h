#ifndef IZUMI__VISUALIZER_H
#define IZUMI__VISUALIZER_H

#include <stdbool.h>

#include "./instruction_table.h"

typedef void * izumi_handle_t;

typedef enum {
    IZUMI_ERROR_KIND__OK = 0,
    IZUMI_ERROR_KIND__IO,
    IZUMI_ERROR_KIND__UNSUPPORTED,
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

izumi_error_t izumi_prepare(const char * trace_path, izumi_handle_t * handle);

izumi_error_t izumi_parse(izumi_handle_t handle, size_t length_hint, izumi_instruction_table_t ** ita_out); // ita_out set to NULL if already fully parsed
izumi_error_t izumi_parse_full(izumi_handle_t handle, izumi_instruction_table_t ** ita_out); // Implicit izumi_done if result is OK

izumi_error_t izumi_done(izumi_handle_t handle); // only frees parsing data
izumi_error_t izumi_free(izumi_handle_t handle); // frees all allocations

#endif
