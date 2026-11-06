#include <izumi/data_structs.h>
#include "test.h"

static void initialization(void) {
    InstructionTableArray array;
    init_InstructionTableArray(&array);
    CHECK(array.qtty_tables == 1);
    CHECK(array.avail_tables == 4);
    for (unsigned i = 0; i < 256; ++i)
        CHECK(!array.tables[0]->content[i].valid);
    for (unsigned i = 1; i < 4; ++i)
        CHECK(array.tables[i] == NULL);
    Instruction *inst = get_instruction(&array, 0);
    CHECK(inst->valid && !inst->finished && !inst->flushed);
    CHECK(inst->qtty_stages == 0 && inst->stages != NULL);
    CHECK(inst->mem_addr == NULL && inst->instruction == NULL);
    CHECK(get_instruction(&array, 0) == inst);
    free_InstructionTableArray(&array);
}

static void sparse_growth(void) {
    InstructionTableArray array = {0};
    const uint64_t ids[] = {0, 255, 256, 4097, 1024, 65535};
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
        get_instruction(&array, ids[i])->finished = true;
    CHECK(array.qtty_tables == 256 && array.avail_tables >= 256);
    CHECK(array.tables[2] == NULL);
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
        CHECK(get_instruction(&array, ids[i])->finished);
    CHECK(!array.tables[1]->content[1].valid);
    free_InstructionTableArray(&array);
}

static void cleanup_and_reuse(void) {
    InstructionTableArray array = {0};
    free_InstructionTableArray(&array);
    Instruction *inst = get_instruction(&array, 300);
    free_Instruction(inst);
    CHECK(!inst->valid && inst->stages == NULL && inst->qtty_stages == 0);
    CHECK(get_instruction(&array, 300)->valid);
    free_InstructionTableArray(&array);
    CHECK(array.tables == NULL && array.qtty_tables == 0 && array.avail_tables == 0);
    free_InstructionTableArray(&array);
    CHECK(get_instruction(&array, 42)->valid);
    free_InstructionTableArray(&array);
}

int main(void) {
    RUN(initialization);
    RUN(sparse_growth);
    RUN(cleanup_and_reuse);
    return EXIT_SUCCESS;
}
