#include <izumi/finder.h>
#include <izumi/parser.h>
#include "test.h"

/* Internal link-visible helper; deliberately not part of the public API. */
extern FindResult check_instruction(InstructionTableArray *, const char *,
                                   FindDataKind, uint64_t, uint64_t);

static void expect(InstructionTableArray *array, const char *pattern,
                   FindDataKind kind, SearchDirection dir, uint64_t start,
                   bool valid, uint64_t position) {
    FindResult result = find(array, pattern, kind, dir, start);
    if (result.valid != valid || result.position != position)
        fprintf(stderr, "pattern='%s', kind=%d, direction=%d, start=%lu: "
                "got (%d, %lu), expected (%d, %lu)\n", pattern, kind, dir,
                (unsigned long)start, result.valid, (unsigned long)result.position,
                valid, (unsigned long)position);
    CHECK(result.valid == valid);
    CHECK(result.position == position);
}

static void exact_matching(void) {
    InstructionTableArray array = {0};
    get_instruction(&array, 0);
    line_of_data(&array, "L\t0\t0\t0x80000000:   add\ta0, a1, a2\n");
    expect(&array, "add", INST, DOWN, 0, true, 0);
    expect(&array, "ad", INST, DOWN, 0, false, 0);
    expect(&array, "addi", INST, DOWN, 0, false, 0);
    expect(&array, "add operands", INST, DOWN, 0, false, 0);
    expect(&array, "", INST, DOWN, 0, false, 0);
    expect(&array, "0x0000000080000000", PC, DOWN, 0, true, 0);
    expect(&array, "0x0000000080000000junk", PC, DOWN, 0, false, 0);
    expect(&array, "0x80000000", PC, DOWN, 0, false, 0);
    get_instruction(&array, 1);
    line_of_data(&array, "L\t1\t0\t   \t\n");
    expect(&array, "missing", INST, DOWN, 1, false, 0);
    expect(&array, "", INST, DOWN, 1, false, 0);
    free_InstructionTableArray(&array);
}

static void directions_and_sparse_tables(void) {
    InstructionTableArray array = {0};
    expect(&array, "add", INST, DOWN, 0, false, 0);
    expect(&array, "add", INST, UP, UINT64_MAX, false, 0);
    const uint64_t ids[] = {0, 255, 256, 4097};
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        char line[80];
        get_instruction(&array, ids[i]);
        snprintf(line, sizeof(line), "L\t%lu\t0\tadd a0, a1, a2\n", (unsigned long)ids[i]);
        line_of_data(&array, line);
    }
    expect(&array, "add", INST, DOWN, 1, true, 255);
    expect(&array, "add", INST, DOWN, 256, true, 256);
    expect(&array, "add", INST, DOWN, 257, true, 4097);
    expect(&array, "add", INST, UP, 4096, true, 256);
    expect(&array, "add", INST, UP, 255, true, 255);
    expect(&array, "add", INST, UP, 0, true, 0);
    expect(&array, "missing", INST, UP, 4097, false, 0);
    expect(&array, "add", INST, DOWN, UINT64_MAX, false, 0);
    expect(&array, "add", INST, UP, UINT64_MAX, true, 4097);
    free_InstructionTableArray(&array);
}

static void single_instruction_checks(void) {
    InstructionTableArray array = {0};
    get_instruction(&array, 1025);
    FindResult result = check_instruction(&array, "nop", INST, 5, 0);
    CHECK(!result.valid && result.position == 0); /* Out of range. */
    result = check_instruction(&array, "nop", INST, 2, 0);
    CHECK(!result.valid && result.position == 0); /* Absent table. */
    result = check_instruction(&array, "nop", INST, 4, 0);
    CHECK(!result.valid && result.position == 0); /* Invalid slot. */
    result = check_instruction(&array, "nop", INST, 4, 1);
    CHECK(!result.valid && result.position == 0); /* Missing label. */
    result = check_instruction(&array, "0x10", PC, 4, 1);
    CHECK(!result.valid && result.position == 0); /* Missing PC. */
    line_of_data(&array, "L\t1025\t0\t0x10: \t nop a0\n");
    result = check_instruction(&array, "nop", INST, 4, 1);
    CHECK(result.valid && result.position == 1025);
    result = check_instruction(&array, "nops", INST, 4, 1);
    CHECK(!result.valid && result.position == 0);
    result = check_instruction(&array, "0x0000000000000010", PC, 4, 1);
    CHECK(result.valid && result.position == 1025);
    result = check_instruction(&array, "nop", (FindDataKind)99, 4, 1);
    CHECK(!result.valid && result.position == 0);
    free_InstructionTableArray(&array);
}

static void empty_and_unlabeled_searches(void) {
    InstructionTableArray array;
    init_InstructionTableArray(&array);
    expect(&array, "nop", INST, UP, 0, false, 0);
    expect(&array, "nop", INST, DOWN, 0, false, 0);
    get_instruction(&array, 255);
    expect(&array, "nop", INST, UP, UINT64_MAX, false, 0);
    expect(&array, "0x0000000000000000", PC, DOWN, 0, false, 0);
    free_InstructionTableArray(&array);
}

int main(void) {
    RUN(exact_matching);
    RUN(directions_and_sparse_tables);
    RUN(single_instruction_checks);
    RUN(empty_and_unlabeled_searches);
    return EXIT_SUCCESS;
}
