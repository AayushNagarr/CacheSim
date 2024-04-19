#include<stdlib.h>
#include<stdio.h>
#include<omp.h>
#include<string.h>
#include<ctype.h>

typedef char byte;

typedef enum state{
    INVALID,
    SHARED,
    EXCLUSIVE,
    MODIFIED
} cstate;

typedef enum op{
    READ,
    WRITE
} op;

struct cache {
    byte address; // This is the address in memory.
    byte value; // This is the value stored in cached memory.
    // State for you to implement MESI protocol.
    cstate state;
};

struct decoded_inst {
    op type; // 0 is RD, 1 is WR
    byte address;
    byte value; // Only used for WR 
};

typedef struct cache cache;
typedef struct decoded_inst decoded;


/*
 * This is a very basic C cache simulator.
 * The input files for each "Core" must be named core_1.txt, core_2.txt, core_3.txt ... core_n.txt
 * Input files consist of the following instructions:
 * - RD <address>
 * - WR <address> <val>
 */

byte * memory;

// Decode instruction lines
decoded decode_inst_line(char * buffer){
    decoded inst;
    char inst_type[2];
    sscanf(buffer, "%s", inst_type);
    if(!strcmp(inst_type, "RD")){
        int addr = 0;
        sscanf(buffer, "%s %d", inst_type, &addr);
        inst.value = -1;
        inst.address = addr;
        inst.type = 0;
    } else if(!strcmp(inst_type, "WR")){

        int addr = 0;
        int val = 0;
        sscanf(buffer, "%s %d %d", inst_type, &addr, &val);
        inst.address = addr;
        inst.value = val;
        inst.type = 1;
    }


    return inst;
}

// Helper function to print the cachelines
void print_cachelines(cache *c, int cache_size) {
  for (int i = 0; i < cache_size; i++) {
    cache cacheline = *(c + i);
    char state[10];
    switch (cacheline.state) {
    case INVALID:
      strcpy(state, "Invalid");
      break;
    case SHARED:
      strcpy(state, "Shared");
      break;
    case EXCLUSIVE:
      strcpy(state, "Exclusive");
      break;
    case MODIFIED:
      strcpy(state, "Modified");
      break;
    }
    printf("\t\tAddress: %d, State: %s, Value: %d\n", cacheline.address, state,
          cacheline.value);
  }
}


// This function implements the mock CPU loop that reads and writes data.
void cpu_loop(int num_threads){
    // Initialize a CPU level cache that holds about 2 bytes of data.
    int cache_size = 2;
    cache ** c = (cache **) malloc(sizeof(cache*) * num_threads);

    for(int i = 0; i< num_threads; i++){
        c[i] = (cache*) malloc(sizeof(cache)*cache_size);
        memset(c[i], 0, sizeof(cache) * cache_size);
    }

    #ifdef DEBUG
    printf("INITIAL");
    for(int i = 0; i < num_threads; i++){
        print_cachelines(c[i], cache_size);
    }
    #endif
    #pragma omp parallel num_threads(num_threads)
    {
    
        
        int core_id = omp_get_thread_num();
        char filepath[50];
        sprintf(filepath, "input_%d.txt", core_id);
        FILE * inst_file = fopen(filepath, "r");
        char inst_line[20];
        
        while (fgets(inst_line, sizeof(inst_line), inst_file)){
            

            decoded inst = decode_inst_line(inst_line);
          
            int hash = inst.address%cache_size;
            cache* cacheline = c[core_id]+hash;

            if(cacheline->address != inst.address || cacheline->state == INVALID){
                
                if(cacheline->state == MODIFIED){
                    memory[cacheline->address] = cacheline->value;
                    cacheline->value = memory[inst.address];
                }

                cacheline->address = inst.address;
                if(inst.type == READ){
                        byte found = 0;
                        for(int i = 0; i < num_threads; i++){
                            cache * ocore = (c[i] + hash);

                            if(i == core_id) continue;
                            if(ocore->address != inst.address) continue;
                            if(ocore->state != INVALID){
                                found = 1;
                                cacheline->state = SHARED;
                                cacheline->value = ocore->value;
                                ocore->state = SHARED;
                                break;
                            }
                            
                        }
                        if(!found){
                        cacheline->value = memory[inst.address];
                        cacheline->state = EXCLUSIVE;
                        }

                }
                else if (inst.type == WRITE){
                        cacheline->value = inst.value;
                        cacheline->state = MODIFIED;
                        for(int i = 0; i < num_threads; i++){
                            if(i == core_id) continue;
                            cache * ocore = (c[i] + hash);
                            if(ocore->address != inst.address) continue;

                            ocore->state = INVALID;
                        }

                }
            }
            switch(inst.type){
                case 0:
                    printf("Core Num : %d - Reading from address %d: %d\n", core_id, cacheline->address, cacheline->value);
                    break;
                
                case 1:
                    printf("Core Num : %d - Writing to address %d: %d\n", core_id, cacheline->address, cacheline->value);
                    break;
            }
            
        
        #pragma omp barrier
        
        }
        
    }
    // Read Input file
    // Decode instructions and execute them.
    for(int i = 0; i < num_threads; i++){
        free(c[i]);
    }
    free(c);
}

int main(int c, char * argv[]){
    // Initialize Global memory
    // Let's assume the memory module holds about 24 bytes of data.
    int memory_size = 24;
    memory = (byte *) malloc(sizeof(byte) * memory_size);
    cpu_loop(2);
    free(memory);
}