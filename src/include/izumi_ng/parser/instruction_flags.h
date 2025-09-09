#ifndef IZUMI__PARSER__INSTRUCTION_FLAGS_H
#define IZUMI__PARSER__INSTRUCTION_FLAGS_H

#include <stdint.h>

#define IZUMI_PARSER_INSTRUCTION_FLAGS_VALID    (1 << 0)
#define IZUMI_PARSER_INSTRUCTION_FLAGS_FINISHED (1 << 1)
#define IZUMI_PARSER_INSTRUCTION_FLAGS_FLUSHED  (1 << 2)
typedef uint8_t izumi_parser_instruction_flags_t;

#endif