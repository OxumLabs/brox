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
    size_t sz;
    size_t used_sz;
} MemBlk;

void h() {
    printf("brox - Memory blocks and program runner with cgroup limits\n");
    printf("Usage: brox <cmd> <prog_name> <size_mb> <debug y/n>\n\n");
    printf("Commands:\n");
    printf("  run <prog_name> <size_mb> <debug y/n>  Run a program in cgroup with memory limit.\n");
    printf("  stress <size_mb> <debug y/n>           Stress test by allocating memory.\n");
    printf("  help                                  Show this help message.\n");
    printf("\nExample usage:\n");
    printf("  brox run 'stress --vm 1 --vm-bytes 512M --timeout 10s' 100 y\n");
    printf("  brox stress 500 y\n");
    printf("  brox help\n");
}

MemBlk *mb_create(size_t sz) {
    MemBlk *blk = (MemBlk *)calloc(1, sizeof(MemBlk));
    if (!blk) {
        perror("[ERR] Allocating MemBlk");
        return NULL;
    }
    blk->ptr = malloc(sz);
    if (!blk->ptr) {
        perror("[ERR] Allocating block memory");
        free(blk);
        return NULL;
    }
    blk->sz = sz;
    blk->used_sz = 0;
    return blk;
}

void mb_free(MemBlk *blk) {
    if (blk) {
        if (blk->ptr) free(blk->ptr);
        free(blk);
    }
}

void dbg_log(const char *msg, int dbg) {
    if (dbg) printf("[DBG] %s\n", msg);
}

int drop_privs(int dbg) {
    struct passwd *pw = getpwnam("nobody");
    if (!pw) {
        fprintf(stderr, "[ERR] Couldn't get 'nobody' user: %s\n", strerror(errno));
        return -1;
    }

    if (setuid(pw->pw_uid) != 0) {
        fprintf(stderr, "[ERR] Couldn't drop privileges: %s\n", strerror(errno));
        return -1;
    }

    if (dbg) printf("[DBG] Dropped to 'nobody' user.\n");
    return 0;
}

int run_prog_with_cgroup(const char *cmd, size_t mem_limit, int dbg) {
    const char *cg_path = "/sys/fs/cgroup/my_cgroup";
    if (mkdir(cg_path, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "[ERR] Creating cgroup dir '%s': %s\n", cg_path, strerror(errno));
        return -1;
    }

    char mem_file[1024];
    snprintf(mem_file, sizeof(mem_file), "%s/memory.max", cg_path);
    FILE *f_mem = fopen(mem_file, "w");
    if (!f_mem) {
        fprintf(stderr, "[ERR] Opening memory limit file '%s': %s\n", mem_file, strerror(errno));
        return -1;
    }
    if (fprintf(f_mem, "%zu", mem_limit) < 0) {
        fprintf(stderr, "[ERR] Writing to '%s': %s\n", mem_file, strerror(errno));
        fclose(f_mem);
        return -1;
    }
    fclose(f_mem);

    char proc_file[1024];
    snprintf(proc_file, sizeof(proc_file), "%s/cgroup.procs", cg_path);
    FILE *f_proc = fopen(proc_file, "w");
    if (!f_proc) {
        fprintf(stderr, "[ERR] Opening cgroup.procs '%s': %s\n", proc_file, strerror(errno));
        return -1;
    }
    if (fprintf(f_proc, "%d", getpid()) < 0) {
        fprintf(stderr, "[ERR] Writing to '%s': %s\n", proc_file, strerror(errno));
        fclose(f_proc);
        return -1;
    }
    fclose(f_proc);

    if (dbg) {
        printf("[DBG] Running cmd: %s\n", cmd);
        printf("[DBG] Memory limit: %zu bytes\n", mem_limit);
    }

    if (drop_privs(dbg) != 0) return -1;

    int res = system(cmd);
    if (res != 0) {
        fprintf(stderr, "[ERR] Command failed with code %d\n", res);
        return -1;
    }

    return res;
}

void stress_test(size_t tot_mem, int dbg) {
    size_t alloc_mem = 0;
    size_t chunk_sz = 1024 * 1024;
    char *mem_blk;
    time_t last_dbg = time(NULL);

    if (dbg) {
        printf("[DBG] Starting stress test for %zu MB...\n", tot_mem / (1024 * 1024));
    }

    while (alloc_mem < tot_mem) {
        mem_blk = (char *)malloc(chunk_sz);
        if (!mem_blk) {
            fprintf(stderr, "[ERR] Memory alloc failed at %zu MB, Total: %zu MB\n", 
                    alloc_mem / (1024 * 1024), alloc_mem / (1024 * 1024));
            return;
        }

        memset(mem_blk, 0, chunk_sz);
        alloc_mem += chunk_sz;
        if (dbg && difftime(time(NULL), last_dbg) >= 1.0) {
            printf("[DBG] Allocated %zu MB, Total: %zu MB\n", 
                   chunk_sz / (1024 * 1024), alloc_mem / (1024 * 1024));
            last_dbg = time(NULL);
        }

        usleep(50000);
    }

    if (dbg) {
        printf("[DBG] Stress test done. Total allocated: %zu MB\n", alloc_mem / (1024 * 1024));
    }
}

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        fprintf(stderr, "[ERR] Must be run as root or with sudo. UID: %d\n", geteuid());
        return -1;
    }

    if (argc < 3) {
        h();
        return -1;
    }

    int dbg = 0;
    if (argc == 5 && strcmp(argv[4], "y") == 0) {
        dbg = 1;
    }

    if (strcmp(argv[1], "help") == 0) {
        h();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 4) {
            h();
            return -1;
        }
        const char *prog_name = argv[2];
        size_t size_mb = atoi(argv[3]);
        if (size_mb == 0) {
            fprintf(stderr, "[ERR] Invalid size: %s\n", argv[3]);
            return -1;
        }
        size_t blk_sz = size_mb * 1024 * 1024;
        MemBlk *blk = mb_create(blk_sz);
        if (!blk) {
            fprintf(stderr, "[ERR] Failed to create block of size %zu bytes\n", blk_sz);
            return -1;
        }
        if (dbg) printf("[DBG] Allocated %zu bytes of memory\n", blk_sz);
        if (run_prog_with_cgroup(prog_name, blk_sz, dbg) != 0) {
            fprintf(stderr, "[ERR] Failed to run program within cgroup.\n");
            mb_free(blk);
            return -1;
        }
        mb_free(blk);
        return 0;
    }

    if (strcmp(argv[1], "stress") == 0) {
        if (argc < 3) {
            h();
            return -1;
        }
        size_t size_mb = atoi(argv[2]);
        if (size_mb == 0) {
            fprintf(stderr, "[ERR] Invalid stress test size: %s\n", argv[2]);
            return -1;
        }
        stress_test(size_mb * 1024 * 1024, dbg);
        return 0;
    }

    return 0;
}
