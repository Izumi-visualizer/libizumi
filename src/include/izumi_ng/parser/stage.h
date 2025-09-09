#ifndef IZUMI__PARSER__STAGE_H
#define IZUMI__PARSER__STAGE_H

#include "../types.h"
#include <stdint.h>

typedef struct {
    uint64_t lane;
    izumi_cycle_t start;
    izumi_cycle_t duration;
    char * name;
} izumi_parser_stage_t;

#endif