#ifndef IZUMI__PARSER__INSTRUCTION_TABLE_H
#define IZUMI__PARSER__INSTRUCTION_TABLE_H

#include "../util/vector.h"

typedef struct {
    izumi_vector_t inner; // Vector of izuimi_parser_instruction
    void * next; // izumi_parser_instruction_table_t *, can't self reference
} izumi_parser_instruction_table_t;

izumi_parser_instruction_table_t * izumi_parser_instruction_table__init();

#endif