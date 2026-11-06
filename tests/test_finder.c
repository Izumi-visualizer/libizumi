#include <izumi/finder.h>
#include <izumi/parser.h>
#include "test.h"

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

int main(void) {
    RUN(exact_matching);
    RUN(directions_and_sparse_tables);
    return EXIT_SUCCESS;
}
