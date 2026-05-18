#ifndef IZUMI__PARSER__INSTRUCTION_H
#define IZUMI__PARSER__INSTRUCTION_H

#include "instruction_flags.h"
#include "../util/vector.h"
#include <stdint.h>

typedef struct {
    izumi_parser_instruction_flags_t flags;
    izumi_vector_t stages;
    uint64_t thread;
    uint64_t address;
    char * disassembly;
    char * tooltip;
} izumi_parser_instruction_t;

#endif