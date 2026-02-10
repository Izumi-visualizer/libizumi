#ifndef IZUMI__INSTRUCTION_H
#define IZUMI__INSTRUCTION_H

#include <stdbool.h>
#include <stddef.h>

#include "./types.h"
#include "./stage.h"
#include "./error.h"

typedef struct {
    uint64_t thread_id;
    uint64_t address;
    char * disassembly;
    char * tooltip;
    bool flushed;
    size_t lane_count;
    size_t first_stage_offset;
    size_t stages_amount;
} izumi_instruction_t;

izumi_error_kind_t izumi_instruction__stage_counts(izumi_instruction_t * instruction, size_t * counts);

#endif
