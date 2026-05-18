#ifndef IZUMI__PARSER__PARSER_H
#define IZUMI__PARSER__PARSER_H

#include "instruction_table.h"

typedef void * izumi_parser_handle_t;

typedef enum {
    IZUMI_PARSER_ERROR_KIND__OK = 0,
    IZUMI_PARSER_ERROR_KIND__IO,
    IZUMI_PARSER_ERROR_KIND__UNSUPPORTED,
    IZUMI_PARSER_ERROR_KIND__PARSER,
    IZUMI_PARSER_ERROR_KIND__UNKNOWN,
} izumi_parser_error_kind_t;

typedef struct {
    union {
        izumi_parser_error_kind_t kind;
        bool present;
    };
    char * details;
} izumi_parser_error_t;

izumi_parser_error_t izumi_parser_prepare(const char * trace_path, izumi_parser_instruction_table_t * first_it, izumi_parser_handle_t handle);

izumi_parser_error_t izumi_parser_parse(izumi_parser_handle_t handle, size_t length_hint);

izumi_parser_error_t izumi_done(izumi_parser_handle_t handle);

#endif