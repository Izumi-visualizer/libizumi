#ifndef IZUMI__INSTRUCTION_TABLE_H
#define IZUMI__INSTRUCTION_TABLE_H

#include <stddef.h>

#include "./instruction.h"

typedef struct {
    size_t length;
    size_t capacity;
    izumi_stage_t stages[];
} izumi_stages_array_t;

typedef struct {
    izumi_stages_array_t *stages_array;
    size_t length;
    size_t capacity;
    izumi_instruction_t instructions[];
} izumi_instruction_table_t;

inline izumi_instruction_t * izumi_instruction_table__get(izumi_instruction_table_t * it, size_t index) {
    return it->instructions[index];
}

#endif
