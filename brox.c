#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>

typedef struct {
    void *ptr;
    size_t size;
    size_t used_size;
} MemoryBlock;

void print_help() {
    printf("cbrox - A program to allocate memory blocks and run programs with cgroup memory limits\n");
    printf("Usage: cbrox <command> <program_name> <size_in_megabytes> <automation y/n> <debug y/n>\n\n");
    printf("Commands:\n");
    printf("  run <program_name> <size_in_megabytes> <automation y/n> <debug y/n> Run a program within a memory-limited cgroup.\n");
    printf("  stress <size_in_megabytes> <debug y/n>               Stress test by allocating memory in chunks.\n");
    printf("  help                   Show this help message.\n");
    printf("\nExample usage:\n");
    printf("  cbrox run 'stress --vm 1 --vm-bytes 512M --timeout 10s' 100 n y\n");
    printf("  cbrox stress 500 y\n");
    printf("  cbrox help\n");
}

MemoryBlock *memory_block_create(size_t size) {
    MemoryBlock *block = (MemoryBlock *)calloc(1, sizeof(MemoryBlock));
    if (!block) return NULL;
    block->ptr = malloc(size);
    if (!block->ptr) {
        free(block);
        return NULL;
    }
    block->size = size;
    block->used_size = 0;
    return block;
}

void memory_block_free(MemoryBlock *block) {
    if (block) {
        if (block->ptr) free(block->ptr);
        free(block);
    }
}

void debug_log(const char *msg, int debug) {
    if (debug) printf("[DEBUG] %s\n", msg);
}

int drop_privileges(int debug) {
    struct passwd *pw = getpwnam("nobody");
    if (!pw) {
        fprintf(stderr, "[ERROR] Failed to get 'nobody' user info.\n");
        return -1;
    }

    if (setuid(pw->pw_uid) != 0) {
        fprintf(stderr, "[ERROR] Failed to drop privileges.\n");
        return -1;
    }

    if (debug) printf("[DEBUG] Dropped privileges to user 'nobody'.\n");
    return 0;
}

int run_program_with_cgroup(const char *command, size_t memory_limit, int debug) {
    const char *cgroup_path = "/sys/fs/cgroup/my_cgroup";
    if (mkdir(cgroup_path, 0755) == -1 && errno != EEXIST) {
        perror("[ERROR] Failed to create cgroup directory");
        return -1;
    }

    char memory_limit_path[1024];
    snprintf(memory_limit_path, sizeof(memory_limit_path), "%s/memory.max", cgroup_path);
    FILE *memory_limit_file = fopen(memory_limit_path, "w");
    if (!memory_limit_file) {
        perror("[ERROR] Failed to open memory limit file");
        return -1;
    }
    if (fprintf(memory_limit_file, "%zu", memory_limit) < 0) {
        fclose(memory_limit_file);
        perror("[ERROR] Failed to set memory limit");
        return -1;
    }
    fclose(memory_limit_file);

    char procs_path[1024];
    snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
    FILE *procs_file = fopen(procs_path, "w");
    if (!procs_file) {
        perror("[ERROR] Failed to open cgroup.procs file");
        return -1;
    }
    if (fprintf(procs_file, "%d", getpid()) < 0) {
        fclose(procs_file);
        perror("[ERROR] Failed to add current process to cgroup");
        return -1;
    }
    fclose(procs_file);

    if (debug) {
        printf("[DEBUG] Running command: %s\n", command);
        printf("[DEBUG] Memory limit set to %zu bytes\n", memory_limit);
    }

    if (drop_privileges(debug) != 0) return -1;

    int result = system(command);
    if (result != 0 && debug) {
        printf("[DEBUG] Command execution failed with exit code %d\n", result);
    }

    return result;
}

void stress_test(size_t total_memory, int debug) {
    size_t allocated_memory = 0;
    size_t chunk_size = 1024 * 1024;
    char *memory_block;
    time_t last_debug_print = time(NULL);

    if (debug) {
        printf("[DEBUG] Starting stress test to allocate %zu MB of memory...\n", total_memory / (1024 * 1024));
    }

    while (allocated_memory < total_memory) {
        memory_block = (char *)malloc(chunk_size);
        if (!memory_block) {
            if (debug) {
                printf("[DEBUG] Memory allocation failed at %zu MB. Total allocated: %zu MB\n", 
                       allocated_memory / (1024 * 1024), allocated_memory / (1024 * 1024));
            }
            return;
        }

        memset(memory_block, 0, chunk_size);
        allocated_memory += chunk_size;
        if (debug && difftime(time(NULL), last_debug_print) >= 1.0) {
            printf("[DEBUG] Allocated %zu MB, Total allocated: %zu MB\n", 
                   chunk_size / (1024 * 1024), allocated_memory / (1024 * 1024));
            last_debug_print = time(NULL);
        }

        usleep(50000);
    }

    if (debug) {
        printf("[DEBUG] Stress test completed. Total allocated memory: %zu MB\n", allocated_memory / (1024 * 1024));
    }
}

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        fprintf(stderr, "[ERROR] This program must be run as root or with sudo privileges.\n");
        return -1;
    }

    if (argc < 3) {
        print_help();
        return -1;
    }

    int debug = 0;
    if (argc == 6 && strcmp(argv[5], "y") == 0) {
        debug = 1;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 5) {
            print_help();
            return -1;
        }
        const char *program_name = argv[2];
        size_t size_in_mb = atoi(argv[3]);
        const char *automation = argv[4];
        if (size_in_mb == 0 || (strcmp(automation, "y") != 0 && strcmp(automation, "n") != 0)) return -1;
        size_t block_size = size_in_mb * 1024 * 1024;
        MemoryBlock *block = memory_block_create(block_size);
        if (!block) return -1;
        if (debug) printf("[DEBUG] Allocated %zu bytes of memory\n", block_size);
        if (run_program_with_cgroup(program_name, block_size, debug) != 0) {
            if (debug) printf("[DEBUG] Program execution within cgroup failed.\n");
            memory_block_free(block);
            return -1;
        }
        memory_block_free(block);
        return 0;
    }

    if (strcmp(argv[1], "stress") == 0) {
        if (argc < 3) {
            print_help();
            return -1;
        }
        size_t size_in_mb = atoi(argv[2]);
        if (size_in_mb == 0) return -1;
        stress_test(size_in_mb * 1024 * 1024, debug);
        return 0;
    }

    return 0;
}
