#ifndef IZUMI__VISUALIZER__STAGE_H
#define IZUMI__VISUALIZER__STAGE_H

#include "../types.h"

typedef struct {
    izumi_cycle_t start;
    izumi_cycle_t duration;
    char * name;
} izumi_stage_t;

#endif