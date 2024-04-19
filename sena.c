#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef char byte;

enum cache_state { Invalid, Shared, Exclusive, Modified };

typedef enum cache_state cache_state;

struct cache_entry {
 byte address; // Memory address.
 byte data;    // Stored value.
 cache_state state; // State for MESI protocol.
};

struct instruction {
 int operation; // 0 for Read, 1 for Write.
 byte address;
 byte data; // Only used for Write.
};

typedef struct cache_entry cache_entry;
typedef struct instruction instruction;

byte *global_memory;

// Function to parse instruction lines.
instruction parse_instruction(char *line) {
 instruction instr;
 char op_type[2];
 sscanf(line, "%s", op_type);
 if (!strcmp(op_type, "RD")) {
    instr.operation = 0;
    int addr = 0;
    sscanf(line, "%s %d", op_type, &addr);
    instr.data = -1;
    instr.address = addr;
 } else if (!strcmp(op_type, "WR")) {
    instr.operation = 1;
    int addr = 0;
    int val = 0;
    sscanf(line, "%s %d %d", op_type, &addr, &val);
    instr.address = addr;
    instr.data = val;
 }
 return instr;
}

// Function to display cache entries.
void display_cache_entries(cache_entry *cache, int cache_size) {
 for (int i = 0; i < cache_size; i++) {
    cache_entry entry = *(cache + i);
    char state_str[10];
    switch (entry.state) {
    case Invalid:
      strcpy(state_str, "Invalid");
      break;
    case Shared:
      strcpy(state_str, "Shared");
      break;
    case Exclusive:
      strcpy(state_str, "Exclusive");
      break;
    case Modified:
      strcpy(state_str, "Modified");
      break;
    }
    printf("\t\tAddress: %d, State: %s, Data: %d\n", entry.address, state_str, entry.data);
 }
}

// Simulate CPU operations.
void simulate_cpu(int num_cores) {
 int cache_size = 2;

 // Allocate memory for cache of each core.
 cache_entry **core_cache = (cache_entry **)malloc(sizeof(cache_entry *) * num_cores);
 for (int i = 0; i < num_cores; i++) {
    *(core_cache + i) = (cache_entry *)malloc(sizeof(cache_entry) * cache_size);
 }

 // Initialize cache state.
 for (int i = 0; i < num_cores; i++) {
    for (int j = 0; j < cache_size; j++) {
      core_cache[i][j].state = Invalid;
    }
 }

#pragma omp parallel num_threads(num_cores)
 {
    int core_id = omp_get_thread_num();
    char file_name[20];
    sprintf(file_name, "input_%d.txt", core_id);
    printf("Processing file: %s\n", file_name);

    FILE *input_file = fopen(file_name, "r");
    char line[20];

    while (fgets(line, sizeof(line), input_file)) {
#pragma omp single
      {
        instruction instr = parse_instruction(line);

        int hash = instr.address % cache_size;

        if (core_cache[core_id][hash].address != instr.address &&
            (core_cache[core_id][hash].state == Modified ||
             core_cache[core_id][hash].state == Shared)) {
          // Flush current cache entry to memory.
          global_memory[core_cache[core_id][hash].address] = core_cache[core_id][hash].data;
          core_cache[core_id][hash].data = global_memory[instr.address];
        }

        if (instr.operation == 1) { // Write operation
          core_cache[core_id][hash].address = instr.address;
          core_cache[core_id][hash].data = instr.data;
          core_cache[core_id][hash].state = Modified;

          // Invalidate other caches if data is not exclusive.
          if (core_cache[core_id][hash].state != Exclusive) {
            for (int i = 0; i < num_cores; i++) {
              if (i == core_id) continue;
              if (core_cache[i][hash].address == instr.address) {
                core_cache[i][hash].state = Invalid;
              }
            }
          }
        } else { // Read operation
          if (core_cache[core_id][hash].address != instr.address ||
              core_cache[core_id][hash].state == Invalid) {
            bool found = false;
            for (int i = 0; i < num_cores; i++) {
              if (i == core_id || core_cache[i][hash].address != instr.address ||
                 core_cache[i][hash].state == Invalid) continue;
              core_cache[core_id][hash] = core_cache[i][hash];
              core_cache[i][hash].state = Shared;
              core_cache[core_id][hash].state = Shared;
              found = true;
            }
            if (!found) {
              core_cache[core_id][hash].data = global_memory[instr.address];
              core_cache[core_id][hash].state = Exclusive;
              core_cache[core_id][hash].address = instr.address;
            }
          }
        }

        switch (instr.operation) {
        case 0:
          printf("Core %d Reading from address %02d: %02d\n", core_id, core_cache[core_id][hash].address, core_cache[core_id][hash].data);
          break;
        case 1:
          printf("Core %d Writing   to address %02d: %02d\n", core_id, core_cache[core_id][hash].address, core_cache[core_id][hash].data);
          break;
        }
      }
#pragma omp barrier
    }
    free(core_cache[core_id]);
 }
 free(core_cache);
}

int main(int argc, char *argv[]) {
 int memory_size = 24;
 global_memory = (byte *)malloc(sizeof(byte) * memory_size);
 simulate_cpu(2);
 free(global_memory);
}
