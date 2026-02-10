#include "izumi/instruction.h"
#include "izumi/stage.h"
#include "izumi/types.h"


typedef struct {
    izumi_cycle_t start;
    izumi_cycle_t duration;
    char *name;
}




typedef struct {
    uint64_t thread_id;
    uint64_t address;
    char * disassembly;
    char * tooltip;
    bool flushed;
    size_t lane_count;
    tmp_stage_t stages[];
} tmp_instruction_t;
