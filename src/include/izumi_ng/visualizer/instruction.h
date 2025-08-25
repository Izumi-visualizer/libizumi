#ifndef IZUMI__VISUALIZER__INSTRUCTION_H
#define IZUMI__VISUALIZER__INSTRUCTION_H

#include <stdbool.h>
#include <stddef.h>

#include "../types.h"
#include "stage.h"

typedef struct {
    uint64_t thread_id;
    uint64_t address;
    char * disassembly;
    char * tooltip;
    bool flushed;
    size_t lane_count;
    izumi_stage_t stages[];
} izumi_instruction_t;

void izumi_instruction__stage_counts(izumi_instruction_t * instruction, size_t * counts);

#endif