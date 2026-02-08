#include "izumi/instruction.h"

izumi_error_kind_t izumi_instruction__stage_counts(izumi_instruction_t *instruction, size_t *counts) {
    if (instruction == NULL) return IZUMI_ERROR_KIND__INVALID;

    izumi_stage_t *stage = instruction->stages;

    *counts = 0;

    for (size_t i = 0; i < instruction->lane_count; i++) {
        while (stage->duration != 0) {
            (*counts)++;
            stage++;
        }

        if (stage->duration == 0) {
            stage++;
        }
    }

    return IZUMI_ERROR_KIND__OK;
}
