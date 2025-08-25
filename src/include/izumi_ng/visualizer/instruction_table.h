#ifndef IZUMI__VISUALIZER__INSTRUCTION_TABLE_H
#define IZUMI__VISUALIZER__INSTRUCTION_TABLE_H

#include <stddef.h>

#include "instruction.h"

typedef struct {
    size_t stride;
    size_t length;
    izumi_instruction_t instructions[];
} izumi_instruction_table_t;

inline izumi_instruction_t * izumi_instruction_table__get(izumi_instruction_table_t * it, size_t index) {
    return (izumi_instruction_t *) &((uint8_t*) it->instructions)[it->stride * index];
}

#endif