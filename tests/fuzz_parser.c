/* One input per process: parse_file() intentionally exits on invalid input. */
#include <inttypes.h>
#include <izumi/parser.h>
#include "test.h"

static void hex_string(const char *text) {
    if (text == NULL) {
        fputc('-', stdout);
        return;
    }
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p)
        printf("%02x", *p);
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    InstructionTableArray array = parse_file(argv[1]);
    for (uint64_t t = 0; t < array.qtty_tables; ++t) {
        if (array.tables[t] == NULL) continue;
        for (uint64_t j = 0; j < 256; ++j) {
            Instruction *inst = &array.tables[t]->content[j];
            if (!inst->valid) continue;
            printf("I %" PRIu64 " %d %d ", t * 256 + j, inst->finished, inst->flushed);
            hex_string(inst->mem_addr);
            putchar(' ');
            hex_string(inst->instruction);
            putchar('\n');
            for (uint64_t s = 0; s < inst->qtty_stages; ++s) {
                Stage *stage = &inst->stages[s];
                CHECK(stage->source == 0);
                CHECK(stage->duration <= UINT64_MAX - stage->cycle);
                CHECK(!inst->finished || !stage->open);
                printf("S %" PRIu64 " %" PRIu64 " %" PRIu64 " %d ",
                       stage->cycle, stage->duration, stage->lane, stage->open);
                hex_string(stage->name);
                putchar('\n');
            }
        }
    }
    free_InstructionTableArray(&array);
    return EXIT_SUCCESS;
}
