#ifndef IZUMI__VISUALIZER__INSTRUCTION_TABLE_ARRAY_H
#define IZUMI__VISUALIZER__INSTRUCTION_TABLE_ARRAY_H

#include <stddef.h>

#include "instruction_table.h"
#include "../util/vector.h"

typedef struct {
    izumi_vector_t inner;
} izumi_instruction_table_array_t;

izumi_instruction_table_array_t izumi_instruction_table_array__init();
void izumi_instruction_table_array__free(izumi_instruction_table_array_t * ita);

#endif