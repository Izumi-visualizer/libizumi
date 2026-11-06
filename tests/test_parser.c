#include <inttypes.h>
#include <unistd.h>
#include <sys/wait.h>
#include <izumi/parser.h>
#include "test.h"

/* Internal link-visible helper; deliberately not part of the public API. */
extern int integer_length(uint64_t);

static void integer_lengths(void) {
    CHECK(integer_length(0) == 1);
    CHECK(integer_length(1) == 1);
    uint64_t power = 10;
    for (int digits = 2; digits <= 20; ++digits) {
        CHECK(integer_length(power - 1) == digits - 1);
        CHECK(integer_length(power) == digits);
        if (digits < 20) power *= 10;
    }
    CHECK(integer_length(UINT64_MAX) == 20);
}

static void cycle_commands(void) {
    uint64_t cycle = 123;
    cycle_set(&cycle, "C=\t0");
    CHECK(cycle == 0);
    cycle_increment(&cycle, "C\t0\n");
    CHECK(cycle == 0);
    cycle_set(&cycle, "C=\t4294967296\r\n");
    CHECK(cycle == UINT64_C(4294967296));
    cycle_increment(&cycle, "C\t4294967297");
    CHECK(cycle == UINT64_C(8589934593));
    cycle_set(&cycle, "C=\t18446744073709551614\n");
    cycle_increment(&cycle, "C\t1");
    CHECK(cycle == UINT64_MAX);
}

static void instruction_commands(void) {
    InstructionTableArray array = {0};
    new_instruction(&array, "I\t1025\t18446744073709551615\t42");
    CHECK(array.qtty_tables == 5 && array.tables[2] == NULL);
    Instruction *inst = &array.tables[4]->content[1];
    CHECK(inst->valid && !inst->finished && !inst->flushed);
    CHECK(inst->qtty_stages == 0 && inst->stages != NULL);
    CHECK(inst->instruction == NULL && inst->mem_addr == NULL);
    new_instruction(&array, "I\t255\t0\t0\n");
    CHECK(array.tables[0]->content[255].valid);
    CHECK(!array.tables[0]->content[254].valid);
    CHECK(inst == get_instruction(&array, 1025));
    free_InstructionTableArray(&array);
}

static void stage_and_retirement_commands(void) {
    InstructionTableArray array = {0};
    new_instruction(&array, "I\t0\t0\t0");
    Instruction *inst = get_instruction(&array, 0);
    new_stage(&array, 10, "S\t0\t18446744073709551615\tExecute\r\n");
    CHECK(inst->qtty_stages == 1);
    CHECK(inst->stages[0].cycle == 10 && inst->stages[0].duration == 0);
    CHECK(inst->stages[0].lane == UINT64_MAX && inst->stages[0].source == 0);
    CHECK(inst->stages[0].open && strcmp(inst->stages[0].name, "Execute") == 0);
    end_stage(&array, 11, "E\t0\t18446744073709551615\tWrongName");
    CHECK(inst->stages[0].open);
    end_stage(&array, 12, "E\t0\t18446744073709551615\tExecute\r\n");
    CHECK(!inst->stages[0].open && inst->stages[0].duration == 2);
    new_stage(&array, 13, "S\t0\t18446744073709551615\tExecute");
    end_stage(&array, 16, "E\t0\t18446744073709551615\tExecute");
    CHECK(inst->stages[0].duration == 2 && inst->stages[1].duration == 3);
    CHECK(!inst->stages[1].open);
    new_stage(&array, 17, "S\t0\t0\tCommit");
    retire_instruction(&array, 20, "R\t0\t999\t1");
    CHECK(inst->finished && inst->flushed);
    CHECK(inst->stages[0].duration == 2 && inst->stages[1].duration == 3);
    CHECK(!inst->stages[2].open && inst->stages[2].duration == 3);
    retire_instruction(&array, 25, "R\t0\t999\t0");
    CHECK(inst->flushed && inst->stages[2].duration == 3);
    free_InstructionTableArray(&array);
}

static InstructionTableArray parse_text(const char *text) {
    char path[] = "izumi-test-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    FILE *file = fdopen(fd, "w");
    CHECK(file != NULL);
    CHECK(fputs(text, file) >= 0);
    CHECK(fclose(file) == 0);
    InstructionTableArray array = parse_file(path);
    CHECK(unlink(path) == 0);
    return array;
}

static void labels(void) {
    InstructionTableArray array = {0};
    Instruction *inst = get_instruction(&array, 0);
    line_of_data(&array, "L\t0\t0\t0x0: nop\n");
    CHECK(strcmp(inst->mem_addr, "0x0000000000000000") == 0);
    CHECK(strcmp(inst->instruction, "nop") == 0);
    line_of_data(&array, "L\t0\t0\t ; extra\n");
    CHECK(strcmp(inst->instruction, "nop ; extra") == 0);
    CHECK(strcmp(inst->mem_addr, "0x0000000000000000") == 0);
    line_of_data(&array, "L\t0\t1\tignored tooltip\n");
    line_of_data(&array, "L\t0\t2\tignored stage tooltip\n");
    line_of_data(&array, "L\t0\t256\tignored unknown label type\n");
    CHECK(strcmp(inst->instruction, "nop ; extra") == 0);
    free_InstructionTableArray(&array);
}

static void label_boundaries(void) {
    const char *labels[] = {"\n", "  add\ta0\r\n", "dead: plain text\n",
                            "0xffffffffffffffff: ret", "FADD.D (iteration 12)\n"};
    const char *expected[] = {"", "  add\ta0", "dead: plain text", "ret",
                              "FADD.D (iteration 12)"};
    InstructionTableArray array = {0};
    for (unsigned i = 0; i < 5; ++i) {
        char line[160];
        Instruction *inst = get_instruction(&array, i);
        snprintf(line, sizeof(line), "L\t%u\t0\t%s", i, labels[i]);
        line_of_data(&array, line);
        CHECK(strcmp(inst->instruction, expected[i]) == 0);
        if (i == 3)
            CHECK(strcmp(inst->mem_addr, "0xffffffffffffffff") == 0);
        else
            CHECK(inst->mem_addr == NULL);
    }
    free_InstructionTableArray(&array);
}

static void lane_timing(void) {
    InstructionTableArray array = parse_text(
        "Kanata\t0004\nC=\t100\nI\t0\t12\t0\n"
        "L\t0\t0\t0x80000000: add a0, a1, a2\nS\t0\t0\tFetch\n"
        "C\t2\nS\t0\t7\tFetch\nC\t1\nS\t0\t0\tExecute\n"
        "E\t0\t0\tFetch\nE\t0\t8\tFetch\nC\t2\nE\t0\t7\tFetch\n"
        "S\t0\t7\tZero\nE\t0\t7\tZero\nC\t1\nE\t0\t7\tZero\n"
        "S\t0\t7\tFinal\nC\t4\nR\t0\t0\t0\n");
    Instruction *inst = get_instruction(&array, 0);
    const uint64_t starts[] = {100, 102, 103, 105, 106};
    const uint64_t durations[] = {3, 3, 7, 0, 4};
    const uint64_t lanes[] = {0, 7, 0, 7, 7};
    CHECK(inst->finished && !inst->flushed && inst->qtty_stages == 5);
    for (unsigned i = 0; i < 5; ++i) {
        CHECK(inst->stages[i].cycle == starts[i]);
        CHECK(inst->stages[i].duration == durations[i]);
        CHECK(inst->stages[i].lane == lanes[i]);
        CHECK(inst->stages[i].source == 0 && !inst->stages[i].open);
    }
    CHECK(strcmp(inst->stages[2].name, "Execute") == 0);
    free_InstructionTableArray(&array);
}

static void sparse_flush_and_unfinished(void) {
    InstructionTableArray array = parse_text(
        "Kanata\t0004\nC=\t4294967296\nI\t4097\t99\t2\n"
        "S\t4097\t0\tLong stage name\nS\t4097\t123\tLong stage name\n"
        "I\t256\t1\t0\nR\t256\t0\t0\nC\t3\nR\t4097\t0\t1\n"
        "I\t0\t0\t0\nS\t0\t0\tF\n");
    CHECK(array.qtty_tables == 17 && array.tables[2] == NULL);
    Instruction *inst = get_instruction(&array, 4097);
    CHECK(inst->finished && inst->flushed && inst->qtty_stages == 2);
    CHECK(inst->stages[0].cycle == UINT64_C(4294967296));
    CHECK(inst->stages[0].duration == 3 && inst->stages[1].duration == 3);
    CHECK(inst->stages[1].lane == 123);
    inst = get_instruction(&array, 256);
    CHECK(inst->finished && !inst->flushed && inst->qtty_stages == 0);
    inst = get_instruction(&array, 0);
    CHECK(!inst->finished && inst->stages[0].open);
    free_InstructionTableArray(&array);
}

static void stage_growth_and_reset(void) {
    InstructionTableArray array = {0};
    Instruction *inst = get_instruction(&array, 0);
    for (unsigned i = 0; i < 35; ++i)
        new_stage(&array, i, "S\t0\t0\tIteration");
    line_of_data(&array, "L\t0\t0\t0x10: nop");
    retire_instruction(&array, 40, "R\t0\t0\t0");
    CHECK(inst->qtty_stages == 35 && inst->stages[34].duration == 6);
    for (unsigned i = 0; i < 34; ++i)
        CHECK(inst->stages[i].duration == 1 && !inst->stages[i].open);
    retire_instruction(&array, 100, "R\t0\t0\t1");
    CHECK(!inst->flushed && inst->stages[34].duration == 6);
    new_instruction(&array, "I\t0\t1\t0");
    CHECK(inst->valid && !inst->finished && !inst->flushed);
    CHECK(inst->qtty_stages == 0 && inst->instruction == NULL && inst->mem_addr == NULL);
    free_InstructionTableArray(&array);
}

static void empty_and_blank_files(void) {
    InstructionTableArray array = parse_text("");
    CHECK(array.qtty_tables == 1 && !array.tables[0]->content[0].valid);
    free_InstructionTableArray(&array);
    array = parse_text("Kanata\t0004\r\n\n\r\nI\t0\t0\t0\r\nL\t0\t0\t\r\nR\t0\t0\t0");
    CHECK(get_instruction(&array, 0)->finished);
    CHECK(strcmp(get_instruction(&array, 0)->instruction, "") == 0);
    free_InstructionTableArray(&array);
}

static void long_labels(void) {
    char text[8300];
    const char *prefix = "Kanata\t0004\nI\t0\t0\t0\nL\t0\t0\t";
    size_t start = strlen(prefix);
    strcpy(text, prefix);
    memset(text + start, 'x', 8192);
    strcpy(text + start + 8192, "\nR\t0\t0\t0\n");
    InstructionTableArray array = parse_text(text);
    Instruction *inst = get_instruction(&array, 0);
    CHECK(strlen(inst->instruction) == 8192);
    CHECK(strspn(inst->instruction, "x") == 8192);
    CHECK(inst->finished && inst->mem_addr == NULL);
    free_InstructionTableArray(&array);
}

/* The current parser API reports input errors by exiting. Isolate these
 * contract tests so a rejected input cannot terminate the test runner. */
static void rejected_inputs(void) {
    const char *inputs[] = {"C=\tbad\n", "C\tbad\n", "I\t0\n",
        "I\t0\t0\t0\nL\t0\n", "I\t0\t0\t0\nS\t0\t0\n",
        "I\t0\t0\t0\nE\t0\t0\n", "I\t0\t0\t0\nR\t0\n",
        "L\t4097\t0\tnop\n", "S\t4097\t0\tF\n",
        "E\t4097\t0\tF\n", "R\t4097\t0\t0\n",
        "I\t4097\t0\t0\nS\t256\t0\tF\n",
        "I\t0\t0\t0\nS\t1\t0\tF\n", "?",
        "C\t-1\n", "C=\t-1\n", "C\t+1\n",
        "C=\t18446744073709551616\n", "C\t1junk\n", "Crap\t1\n",
        "C=\t18446744073709551615\nC\t1\n", "C=\t10\nC=\t9\n",
        "I\t-1\t0\t0\n", "I\t18446744073709551616\t0\t0\n",
        "I\t0\t0\t0\tjunk\n", "I\t0\t-1\t0\n", "I\t0\t0\t-1\n",
        "I\t0\t0\t0\nL\t0\t4294967296\twrapped type\n",
        "I\t0\t0\t0\nL\t0\t-1\tnegative type\n",
        "I\t0\t0\t0\nS\t0\t-1\tF\n",
        "I\t0\t0\t0\nE\t0\t-1\tF\n",
        "I\t0\t0\t0\nR\t0\t0\t2\n",
        "I\t0\t0\t0\nR\t0\t0\t-1\n",
        "I\t0\t0\t0\nR\t0\t0\t0junk\n",
        "I\t0\t0\t0\nR\t0\t0\t0\nS\t0\t0\tF\n",
        "I\t0\t0\t0\nS\t0\t0\tF\nR\t0\t0\t1\nS\t0\t0\tX\n",
        "I\t0\t0\t0\nR\t0\t0\t0\nE\t0\t0\tF\n",
        "I\t0\t0\t0\nR\t0\t0\t0\nL\t0\t0\tnop\n", NULL};
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        char path[] = "izumi-invalid-XXXXXX";
        int fd = mkstemp(path);
        CHECK(fd >= 0);
        FILE *file = fdopen(fd, "w");
        CHECK(file != NULL);
        if (inputs[i]) CHECK(fputs(inputs[i], file) >= 0);
        CHECK(fclose(file) == 0);
        if (!inputs[i]) CHECK(unlink(path) == 0);
        CHECK(fflush(NULL) == 0);
        pid_t child = fork();
        CHECK(child >= 0);
        if (child == 0) {
            CHECK(freopen("/dev/null", "w", stderr) != NULL);
            InstructionTableArray array = parse_file(path);
            free_InstructionTableArray(&array);
            exit(EXIT_SUCCESS);
        }
        int status;
        CHECK(waitpid(child, &status, 0) == child);
        if (inputs[i]) CHECK(unlink(path) == 0);
        CHECK(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
    }
}

static void decreasing_stage_cycles(void) {
    for (unsigned command = 0; command < 3; ++command) {
        CHECK(fflush(NULL) == 0);
        pid_t child = fork();
        CHECK(child >= 0);
        if (child == 0) {
            CHECK(freopen("/dev/null", "w", stderr) != NULL);
            InstructionTableArray array = {0};
            new_instruction(&array, "I\t0\t0\t0");
            new_stage(&array, 10, "S\t0\t0\tF");
            if (command == 0) new_stage(&array, 9, "S\t0\t0\tX");
            if (command == 1) end_stage(&array, 9, "E\t0\t0\tF");
            if (command == 2) retire_instruction(&array, 9, "R\t0\t0\t0");
            free_InstructionTableArray(&array);
            exit(EXIT_SUCCESS);
        }
        int status;
        CHECK(waitpid(child, &status, 0) == child);
        CHECK(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
    }
}

static void embedded_nul(void) {
    char path[] = "izumi-nul-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    const char data[] = "I\t0\t0\t0\0hidden\n";
    CHECK(write(fd, data, sizeof(data) - 1) == (ssize_t)(sizeof(data) - 1));
    CHECK(close(fd) == 0);
    CHECK(fflush(NULL) == 0);
    pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        CHECK(freopen("/dev/null", "w", stderr) != NULL);
        InstructionTableArray array = parse_file(path);
        free_InstructionTableArray(&array);
        exit(EXIT_SUCCESS);
    }
    int status;
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(unlink(path) == 0);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE);
}

int main(void) {
    RUN(integer_lengths);
    RUN(cycle_commands);
    RUN(instruction_commands);
    RUN(stage_and_retirement_commands);
    RUN(labels);
    RUN(label_boundaries);
    RUN(lane_timing);
    RUN(sparse_flush_and_unfinished);
    RUN(stage_growth_and_reset);
    RUN(empty_and_blank_files);
    RUN(long_labels);
    RUN(rejected_inputs);
    RUN(decreasing_stage_cycles);
    RUN(embedded_nul);
    return EXIT_SUCCESS;
}
