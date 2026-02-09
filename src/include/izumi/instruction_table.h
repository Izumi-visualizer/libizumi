#ifndef IZUMI__INSTRUCTION_TABLE_H
#define IZUMI__INSTRUCTION_TABLE_H

#include <stddef.h>

#include "./instruction.h"

typedef struct {
    size_t length;
    izumi_instruction_t instructions[];
} izumi_instruction_table_t;

typedef struct {
    size_t length;
    izumi_stage_t stas[];
} izumi_stages_blob_t;

inline izumi_instruction_t * izumi_instruction_table__get(izumi_instruction_table_t * it, size_t index) {
    return (izumi_instruction_t *) &((uint8_t*) it->instructions)[it->stride * index];
}

#endif
