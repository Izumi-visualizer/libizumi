#include "unity.h"
#include "izumi/instruction.h"
#include "izumi/stage.h"
#include "izumi/error.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {

}

void tearDown(void) {

}

izumi_instruction_t * izumi_instruction_create(size_t stage_count) {
    return malloc(sizeof(izumi_instruction_t) + stage_count * sizeof(izumi_stage_t));
}

void test_instruction_stages_count_no_lanes(void) {
    izumi_instruction_t *instruction = izumi_instruction_create(6);

    instruction->thread_id = 0;
    instruction->address = 0x0000000080001888;
    instruction->disassembly = "add x1, x2, x3";
    instruction->tooltip = "amogus";
    instruction->flushed = false;
    instruction->lane_count = 0;

    size_t i = 0;

    izumi_stage_t stages[1];

    char *name = "F1";

    stages[0].start = 5;
    stages[0].duration = 0;
    stages[0].name = name;

    memcpy(&instruction->stages, stages, sizeof(izumi_stage_t));

    uint64_t count = 0;

    izumi_error_kind_t res = izumi_instruction__stage_counts(instruction, &count);

    free(instruction);

    if (res != IZUMI_ERROR_KIND__OK) {
        TEST_ASSERT_TRUE(1==0);
    } else {
        TEST_ASSERT_EQUAL_size_t(0, count);
    }
}

void test_instruction_stages_count_no_lanes_with_info(void) {
    izumi_instruction_t *instruction = izumi_instruction_create(6);

    instruction->thread_id = 0;
    instruction->address = 0x0000000080001888;
    instruction->disassembly = "add x1, x2, x3";
    instruction->tooltip = "amogus";
    instruction->flushed = false;
    instruction->lane_count = 0;

    size_t i = 0;

    izumi_stage_t stages[6];

    char *name = "F1";

    for (; i < 5; i++) {
        stages[i].start = i;
        stages[i].duration = 1;
        stages[i].name = name;
    }

    stages[5].start = 5;
    stages[5].duration = 0;
    stages[5].name = name;

    memcpy(&instruction->stages, stages, sizeof(izumi_stage_t)*6);

    uint64_t count = 0;

    izumi_error_kind_t res = izumi_instruction__stage_counts(instruction, &count);

    free(instruction);

    if (res != IZUMI_ERROR_KIND__OK) {
        TEST_ASSERT_TRUE(1==0);
    } else {
        TEST_ASSERT_EQUAL_size_t(0, count);
    }
}


void test_instruction_stages_count_single_lane(void) {
    izumi_instruction_t *instruction = izumi_instruction_create(6);

    instruction->thread_id = 0;
    instruction->address = 0x0000000080001888;
    instruction->disassembly = "add x1, x2, x3";
    instruction->tooltip = "amogus";
    instruction->flushed = false;
    instruction->lane_count = 1;

    size_t i = 0;

    izumi_stage_t stages[6];

    char *name = "F1";

    for (; i < 5; i++) {
        stages[i].start = i;
        stages[i].duration = 1;
        stages[i].name = name;
    }

    stages[5].start = 5;
    stages[5].duration = 0;
    stages[5].name = name;

    memcpy(&instruction->stages, stages, sizeof(izumi_stage_t)*6);

    uint64_t count = 0;

    izumi_error_kind_t res = izumi_instruction__stage_counts(instruction, &count);

    free(instruction);

    if (res != IZUMI_ERROR_KIND__OK) {
        TEST_ASSERT_TRUE(1==0);
    } else {
        TEST_ASSERT_EQUAL_size_t(5, count);
    }
}

void test_instruction_stages_count_multiple_lane(void) {
    izumi_instruction_t *instruction = izumi_instruction_create(12);

    instruction->thread_id = 0;
    instruction->address = 0x0000000080001888;
    instruction->disassembly = "add x1, x2, x3";
    instruction->tooltip = "amogus";
    instruction->flushed = false;
    instruction->lane_count = 2;

    size_t i = 0;

    izumi_stage_t stages[12];

    char *name = "F1";

    for (; i < 4; i++) {
        stages[i].start = i;
        stages[i].duration = 1;
        stages[i].name = name;
    }

    stages[4].start = 4;
    stages[4].duration = 0;
    stages[4].name = name;

    i++;

    for (; i < 11; i++) {
        stages[i].start = i;
        stages[i].duration = 1;
        stages[i].name = name;
    }

    stages[11].start = 11;
    stages[11].duration = 0;
    stages[11].name = name;



    memcpy(&instruction->stages, stages, sizeof(izumi_stage_t)*12);

    uint64_t count = 0;

    izumi_error_kind_t res = izumi_instruction__stage_counts(instruction, &count);

    free(instruction);

    if (res != IZUMI_ERROR_KIND__OK) {
        TEST_ASSERT_TRUE(1==0);
    } else {
        TEST_ASSERT_EQUAL_size_t(10, count);
    }
}

void test_instruction_stages_count_multiple_lane_only_one_valid(void) {
    izumi_instruction_t *instruction = izumi_instruction_create(12);

    instruction->thread_id = 0;
    instruction->address = 0x0000000080001888;
    instruction->disassembly = "add x1, x2, x3";
    instruction->tooltip = "amogus";
    instruction->flushed = false;
    instruction->lane_count = 1;

    size_t i = 0;

    izumi_stage_t stages[12];

    char *name = "F1";

    for (; i < 4; i++) {
        stages[i].start = i;
        stages[i].duration = 1;
        stages[i].name = name;
    }

    stages[4].start = 4;
    stages[4].duration = 0;
    stages[4].name = name;

    i++;

    for (; i < 6; i++) {
        stages[i].start = i;
        stages[i].duration = 1;
        stages[i].name = name;
    }

    stages[11].start = 11;
    stages[11].duration = 0;
    stages[11].name = name;



    memcpy(&instruction->stages, stages, sizeof(izumi_stage_t)*12);

    uint64_t count = 0;

    izumi_error_kind_t res = izumi_instruction__stage_counts(instruction, &count);

    free(instruction);

    if (res != IZUMI_ERROR_KIND__OK) {
        TEST_ASSERT_TRUE(1==0);
    } else {
        TEST_ASSERT_EQUAL_size_t(4, count);
    }
}

void test_instruction_stages_count_null_ptr(void) {
    uint64_t count = 0;

    izumi_error_kind_t res = izumi_instruction__stage_counts(NULL, &count);

    TEST_ASSERT_TRUE(res == IZUMI_ERROR_KIND__INVALID);
}



int main() {
    UNITY_BEGIN();
    RUN_TEST(test_instruction_stages_count_no_lanes);
    RUN_TEST(test_instruction_stages_count_no_lanes_with_info);
    RUN_TEST(test_instruction_stages_count_single_lane);
    RUN_TEST(test_instruction_stages_count_multiple_lane);
    RUN_TEST(test_instruction_stages_count_multiple_lane_only_one_valid);
    RUN_TEST(test_instruction_stages_count_null_ptr);

    return UNITY_END();
}
