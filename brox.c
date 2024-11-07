#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

typedef struct {
    void *ptr;
    size_t size;
    size_t used_size;
} MemoryBlock;

void print_help() {
    printf("cbrox - A program to allocate memory blocks and run programs with cgroup memory limits\n");
    printf("Usage: cbrox <command> <program_name> <size_in_megabytes> <automation y/n>\n\n");
    printf("Commands:\n");
    printf("  run <program_name> <size_in_megabytes> <automation y/n>  Run a program within a memory-limited cgroup.\n");
    printf("    <program_name>      The command or program to run.\n");
    printf("    <size_in_megabytes> The amount of memory (in MB) to allocate for the program.\n");
    printf("    <automation y/n>    Flag indicating whether to increase memory automatically as needed (y for yes, n for no).\n");
    printf("  stress <size_in_megabytes>               Stress test by allocating memory in chunks.\n");
    printf("    <size_in_megabytes> The amount of memory (in MB) to allocate for the stress test.\n");
    printf("  help                   Show this help message.\n");
    printf("\nExample usage:\n");
    printf("  cbrox run 'stress --vm 1 --vm-bytes 512M --timeout 10s' 100 n\n");
    printf("  cbrox stress 500\n");
    printf("  cbrox help\n");
}

MemoryBlock *memory_block_create(size_t size) {
    if (size == 0) {
        fprintf(stderr, "Error: Memory block size must be greater than zero.\n");
        return NULL;
    }

    MemoryBlock *block = (MemoryBlock *)calloc(1, sizeof(MemoryBlock));
    if (!block) {
        perror("Error allocating memory for MemoryBlock structure");
        return NULL;
    }

    block->ptr = malloc(size);
    if (!block->ptr) {
        perror("Error allocating memory for block->ptr");
        free(block);
        return NULL;
    }

    block->size = size;
    block->used_size = 0;

    return block;
}

void memory_block_free(MemoryBlock *block) {
    if (block) {
        if (block->ptr) {
            free(block->ptr);
        }
        free(block);
    }
}

int memory_block_write(MemoryBlock *block, const void *data, size_t data_size) {
    if (!block) {
        fprintf(stderr, "Error: Memory block is NULL.\n");
        return -1;
    }
    if (!data) {
        fprintf(stderr, "Error: Data to write is NULL.\n");
        return -1;
    }
    if (data_size > (block->size - block->used_size)) {
        fprintf(stderr, "Error: Insufficient space in memory block. Available space: %zu bytes\n", block->size - block->used_size);
        return -1;
    }

    memcpy((unsigned char *)block->ptr + block->used_size, data, data_size);
    block->used_size += data_size;
    return 0;
}

void *memory_block_read(MemoryBlock *block, size_t *data_size) {
    if (!block) {
        fprintf(stderr, "Error: Memory block is NULL.\n");
        if (data_size) *data_size = 0;
        return NULL;
    }
    if (block->used_size == 0) {
        fprintf(stderr, "Error: No data in memory block to read.\n");
        if (data_size) *data_size = 0;
        return NULL;
    }

    if (data_size) *data_size = block->used_size;
    return block->ptr;
}

int run_program_with_cgroup(const char *command, size_t memory_limit) {
    const char *cgroup_path = "/sys/fs/cgroup/my_cgroup";

    if (mkdir(cgroup_path, 0755) == -1 && errno != EEXIST) {
        perror("Error creating cgroup directory");
        return -1;
    }

    char memory_limit_path[1024];
    snprintf(memory_limit_path, sizeof(memory_limit_path), "%s/memory.max", cgroup_path);
    FILE *memory_limit_file = fopen(memory_limit_path, "w");
    if (!memory_limit_file) {
        perror("Error opening memory limit file (memory.max)");
        return -1;
    }

    if (fprintf(memory_limit_file, "%zu", memory_limit) < 0) {
        perror("Error writing to memory limit file");
        fclose(memory_limit_file);
        return -1;
    }
    fclose(memory_limit_file);

    char procs_path[1024];
    snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
    FILE *procs_file = fopen(procs_path, "w");
    if (!procs_file) {
        perror("Error opening cgroup.procs file");
        return -1;
    }

    if (fprintf(procs_file, "%d", getpid()) < 0) {
        perror("Error writing to cgroup.procs file");
        fclose(procs_file);
        return -1;
    }
    fclose(procs_file);

    printf("Running command: %s with memory limit of %zu bytes...\n", command, memory_limit);
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Error: Command execution failed with exit code %d.\n", result);
    }
    return result;
}

void stress_test(size_t total_memory) {
    size_t allocated_memory = 0;
    size_t chunk_size = 1024 * 1024;  // 1MB per chunk
    char *memory_block;

    printf("Starting stress test to allocate %zu MB of memory...\n", total_memory / (1024 * 1024));

    while (allocated_memory < total_memory) {
        memory_block = (char *)malloc(chunk_size);
        if (!memory_block) {
            fprintf(stderr, "Error: Memory allocation failed at %zu MB. Total allocated: %zu MB\n", 
                    allocated_memory / (1024 * 1024), allocated_memory / (1024 * 1024));
            return;
        }

        memset(memory_block, 0, chunk_size);  // Fill the memory

        allocated_memory += chunk_size;
        printf("Allocated %zu MB, Total allocated: %zu MB\n", chunk_size / (1024 * 1024), allocated_memory / (1024 * 1024));

        usleep(50000);  // Sleep for 50ms to simulate work
    }

    printf("Stress test completed. Total allocated memory: %zu MB\n", allocated_memory / (1024 * 1024));
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error: Invalid arguments.\n");
        print_help();
        return -1;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "run") != 0 && strcmp(argv[1], "stress") != 0) {
        fprintf(stderr, "Error: Invalid command. Valid commands are 'run' or 'stress'.\n");
        print_help();
        return -1;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Error: Invalid arguments for 'run' command.\n");
            print_help();
            return -1;
        }

        const char *program_name = argv[2];
        size_t size_in_mb = atoi(argv[3]);
        const char *automation = argv[4];

        if (size_in_mb == 0) {
            fprintf(stderr, "Error: Memory size must be greater than zero.\n");
            return -1;
        }

        if (strcmp(automation, "y") != 0 && strcmp(automation, "n") != 0) {
            fprintf(stderr, "Error: Automation flag must be 'y' or 'n'.\n");
            return -1;
        }

        size_t block_size = size_in_mb * 1024 * 1024;
        MemoryBlock *block = memory_block_create(block_size);
        if (!block) {
            fprintf(stderr, "Error: Failed to allocate memory block.\n");
            return -1;
        }

        if (run_program_with_cgroup(program_name, block_size) != 0) {
            fprintf(stderr, "Error: Program execution within cgroup failed.\n");
            memory_block_free(block);
            return -1;
        }

        memory_block_free(block);
        return 0;
    }

    if (strcmp(argv[1], "stress") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: Invalid arguments for 'stress' command.\n");
            print_help();
            return -1;
        }

        size_t size_in_mb = atoi(argv[2]);

        if (size_in_mb == 0) {
            fprintf(stderr, "Error: Stress test size must be greater than zero.\n");
            return -1;
        }

        stress_test(size_in_mb * 1024 * 1024);
        return 0;
    }

    return 0;
}
