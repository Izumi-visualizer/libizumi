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

static void table_initialization_and_cleanup(void) {
    InstructionTable *table = malloc(sizeof(*table));
    CHECK(table != NULL);
    memset(table, 0xa5, sizeof(*table));
    init_InstructionTable(table);
    for (unsigned i = 0; i < 256; ++i)
        CHECK(!table->content[i].valid);
    /* Include both end slots and an interior slot in table-owned cleanup. */
    init_Instruction(&table->content[0]);
    init_Instruction(&table->content[127]);
    init_Instruction(&table->content[255]);
    free_InstructionTable(table);
    free_InstructionTable(NULL);
}

static void explicit_capacity_growth(void) {
    InstructionTableArray array;
    init_InstructionTableArray(&array);
    InstructionTable *first = array.tables[0];
    Instruction *inst = get_instruction(&array, 255);
    inst->flushed = true;
    for (unsigned growth = 0; growth < 3; ++growth) {
        uint64_t old_capacity = array.avail_tables;
        increase_InstructionTableArray(&array);
        CHECK(array.avail_tables == old_capacity * 2);
        CHECK(array.qtty_tables == 1 && array.tables[0] == first);
        CHECK(get_instruction(&array, 255) == inst && inst->flushed);
        for (uint64_t i = 1; i < array.avail_tables; ++i)
            CHECK(array.tables[i] == NULL);
    }
    free_InstructionTableArray(&array);
}

static void instruction_initialization_and_cleanup(void) {
    Instruction inst;
    memset(&inst, 0xa5, sizeof(inst));
    init_Instruction(&inst);
    CHECK(inst.valid && !inst.finished && !inst.flushed);
    CHECK(inst.qtty_stages == 0 && inst.stages != NULL);
    CHECK(inst.mem_addr == NULL && inst.instruction == NULL);
    inst.mem_addr = strdup("0x0000000000000010");
    inst.instruction = strdup("nop");
    CHECK(inst.mem_addr != NULL && inst.instruction != NULL);
    inst.qtty_stages = 2;
    inst.stages[0] = (Stage){.name = strdup("Fetch")};
    inst.stages[1] = (Stage){.name = strdup("Execute")};
    CHECK(inst.stages[0].name != NULL && inst.stages[1].name != NULL);
    free_Instruction(&inst);
    CHECK(!inst.valid && !inst.finished && !inst.flushed);
    CHECK(inst.qtty_stages == 0 && inst.stages == NULL);
    CHECK(inst.mem_addr == NULL && inst.instruction == NULL);
    free_Instruction(&inst);
}

static void stage_cleanup(void) {
    Stage stage = {.cycle = 17, .duration = 3, .lane = 7, .source = 2,
                   .name = strdup("Execute")};
    CHECK(stage.name != NULL);
    free_Stage(&stage);
    CHECK(stage.name == NULL);
    CHECK(stage.cycle == 17 && stage.duration == 3);
    CHECK(stage.lane == 7 && stage.source == 2);
    free_Stage(&stage);
}

int main(void) {
    RUN(initialization);
    RUN(sparse_growth);
    RUN(cleanup_and_reuse);
    RUN(table_initialization_and_cleanup);
    RUN(explicit_capacity_growth);
    RUN(instruction_initialization_and_cleanup);
    RUN(stage_cleanup);
    return EXIT_SUCCESS;
}
