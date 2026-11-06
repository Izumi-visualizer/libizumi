/*
 * This file is part of Izumi.
 *
 * Izumi is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Izumi is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Izumi. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <izumi/parser.h>

#include <assert.h>

int integer_length(u_int64_t number) {
    int length = 0;

    while (number > 0) {
        number /= 10;
        length++;
    }

    return length;
}

void cycle_set(uint64_t *cycle, char *line) {
    uint64_t new_cycle;

    if (sscanf(line, "C=\t%lu", &new_cycle) != 1) {
        fprintf(stderr, "Error: Could not read cycle (C=)\n");
        exit(1);
    }

    *cycle = new_cycle;
}

void cycle_increment(u_int64_t *cycle, char *line) {
    u_int64_t qtty_cycles;

    if (sscanf(line, "C\t%lu", &qtty_cycles) != 1) {
        fprintf(stderr, "Error: Could not read cycles (C)\n");
        exit(1);
    }

    *cycle += qtty_cycles;
}

void new_instruction(InstructionTableArray *tables_array, char *line) {
    u_int64_t id_file, id_sim, id_thread;

    if (sscanf(line, "I\t%lu\t%lu\t%lu", &id_file, &id_sim, &id_thread) != 3) {
        fprintf(stderr, "Error: Could not read instruction\n");
        exit(1);
    }

    // Create a new instruction. Sparse core IDs can skip several tables or
    // arrive out of order; get_instruction() grows the array safely.
    Instruction instr_new;
    init_Instruction(&instr_new);

    Instruction *instr = get_instruction(tables_array, id_file);
    if (instr->valid)
        free_Instruction(instr);
    *instr = instr_new;
}

static Instruction *existing_instruction(InstructionTableArray *array, uint64_t id) {
    if (array->tables == NULL || id/256 >= array->qtty_tables ||
        array->tables[id/256] == NULL ||
        !array->tables[id/256]->content[id%256].valid) {
        fprintf(stderr, "Error: Unknown instruction ID %" PRIu64 "\n", id);
        exit(1);
    }
    return &array->tables[id/256]->content[id%256];
}

void line_of_data(InstructionTableArray *tables_array, char *line) {
    const uint8_t LABEL_TYPE_SIDEBAR = 0;

    uint64_t id;
    unsigned int type;
    int label_start = 0;

    // sscanf("L {id} {type} {label...}")
    // Consume exactly one field separator after TYPE. A whitespace directive
    // would eat leading label whitespace, including empty labels' newlines.
    if (sscanf(line, "L\t%" SCNu64 "\t%u%n", &id, &type, &label_start) != 2 ||
        (line[label_start] != '\t' && line[label_start] != ' ')) {
        fprintf(stderr, "Error: Could not read data (L)\n");
        exit(1);
    }

    // We can only handle sidebar labels, so we skip other types
    if (type != LABEL_TYPE_SIDEBAR) return;

    // Get instruction and label pointers
    Instruction *instruct = existing_instruction(tables_array, id);
    char *label = line + label_start + 1;

    // Optimistically parse as a commit log entry
    uint64_t address;
    int disassembly_start = 0; // if misspredicted, disassembly == label

    // sscanf("{address}: {disassembly...}")
    // The %n conversion does not add to sscanf's assignment count, so check
    // disassembly_start to require the ':' separator. Labels without it (for
    // example the CVFPU trace labels) have no instruction address.
    if (instruct->instruction == NULL && strncmp(label, "0x", 2) == 0 &&
        sscanf(label, "%" SCNx64 ": %n", &address, &disassembly_start) == 1
        && disassembly_start != 0) {
        // Instruction address is known, write it
        instruct->mem_addr = malloc(19);

        // Standarize to fully padded addresses
        snprintf(instruct->mem_addr, 19, "0x%016" PRIx64, address);
    }

    // Copy the disassembly (or full label)
    char * disassembly = label + disassembly_start;
    size_t len = strlen(disassembly);
    if (len > 0 && disassembly[len - 1] == '\n') --len;
    if (len > 0 && disassembly[len - 1] == '\r') --len;
    size_t old_len = instruct->instruction ? strlen(instruct->instruction) : 0;
    instruct->instruction = realloc(instruct->instruction, old_len + len + 1);
    memcpy(instruct->instruction + old_len, disassembly, len);
    instruct->instruction[old_len + len] = '\0';
}

void new_stage(InstructionTableArray *tables_array, u_int64_t cycle, char *line) {
    char *stage_name;
    u_int64_t instr_id, stage_id;

    if (sscanf(line, "S\t%lu\t%lu\t%m[^\r\n]", &instr_id, &stage_id, &stage_name) != 3) {
        fprintf(stderr, "Error: Could not read stage\n");
        exit(1);
    }

    // Get the instruction
    Instruction *instr = existing_instruction(tables_array, instr_id);

    // A new stage implicitly ends the previous stage on this lane only.
    // Stages on other lanes continue independently (e.g. pipeline + stalls).
    for (uint64_t i = 0; i < instr->qtty_stages; ++i) {
        Stage *previous = &instr->stages[i];
        if (previous->open && previous->lane == stage_id) {
            previous->duration = cycle - previous->cycle;
            previous->open = false;
        }
    }

    // Resize the stages array if necessary
    if (instr->qtty_stages % 10 == 0) {
        instr->stages = realloc(instr->stages, (instr->qtty_stages + 10) * sizeof(Stage));
    }

    Stage *stage = &instr->stages[instr->qtty_stages];
    instr->qtty_stages++;

    stage->name = stage_name;
    stage->cycle = cycle;
    stage->lane = stage_id;
    stage->source = 0;
    stage->open = true;
    stage ->duration = 0;
}

void end_stage(InstructionTableArray *tables_array, u_int64_t cycle, char *line) {
    u_int64_t instr_id, stage_id;
    char *stage_name = NULL;

    if (sscanf(line, "E\t%lu\t%lu\t%m[^\r\n]", &instr_id, &stage_id, &stage_name) != 3) {
        fprintf(stderr, "Error: Could not read end stage\n");
        exit(1);
    }

    // Get the instruction
    Instruction *instr = existing_instruction(tables_array, instr_id);

    // E refers to a stage on this exact lane. Never close a same-named stage
    // on a different lane, or modify a stage that was explicitly closed.
    Stage *stage = NULL;

    for (uint64_t i = instr->qtty_stages; i > 0; i--) {
        Stage *candidate = &instr->stages[i - 1];
        if (candidate->open && candidate->lane == stage_id &&
            strcmp(candidate->name, stage_name) == 0) {
            stage = candidate;
            break;
        }
    }

    if (stage != NULL) {
        stage->duration = cycle - stage->cycle;
        stage->open = false;
    }

    free(stage_name);
}

void retire_instruction(InstructionTableArray *tables_array, u_int64_t cycle, char *line) {
    u_int64_t instr_id, retire_id, type;

    if (sscanf(line, "R\t%lu\t%lu\t%lu", &instr_id, &retire_id, &type) != 3) {
        fprintf(stderr, "Error: Could not read retire\n");
        exit(1);
    }


    // Get the instruction
    Instruction *instr = existing_instruction(tables_array, instr_id);

    if (!instr->finished) {
        // Retirement/flush ends every still-open lane. Explicit E boundaries
        // must not be extended to retirement, including zero-duration stages.
        for (uint64_t i = 0; i < instr->qtty_stages; ++i) {
            Stage *stage = &instr->stages[i];
            if (stage->open) {
                stage->duration = cycle - stage->cycle;
                stage->open = false;
            }
        }
        instr->finished = true;
        instr->flushed = (type == 1);
    }
}


InstructionTableArray parse_file(const char *file_name) {
    FILE *file = fopen(file_name, "r");

    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", file_name);
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Initialization of the tables array
    InstructionTableArray tables_array;

    init_InstructionTableArray(&tables_array);

    u_int64_t cycle = 0;

    // Parse the file
    while ((read = getline(&line, &len, file)) != -1) {
        char command;
        char command_ext;

        if (strspn(line, " \t\r\n") == (size_t)read)
            continue;

        if (sscanf(line, "%c%c", &command, &command_ext) != 2) {
            fprintf(stderr, "Error: Could not read command\n");
            exit(1);
        }

        switch (command) {
            case 'C':
                if (command_ext == '=') cycle_set(&cycle, line);
                else cycle_increment(&cycle, line);
                break;
            case 'I':
                new_instruction(&tables_array, line);
                break;
            case 'L':
                line_of_data(&tables_array, line);
                break;
            case 'S':
                new_stage(&tables_array, cycle, line);
                break;
            case 'E':
                end_stage(&tables_array, cycle, line);
                break;
            case 'R':
                retire_instruction(&tables_array, cycle, line);
                break;
            default:
                break;
        }
    }

    fclose(file);

    free(line);
    line = NULL;

    return tables_array;
}
