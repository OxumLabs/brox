
# brox - Memory Block Manager and Program Runner with Cgroup Limits

## Overview

**brox** is a C program that can do two main things:
1. **Run another program in a controlled environment** using a `cgroup` to limit its memory.
2. **Stress-test the system’s memory** by repeatedly allocating memory.

This program is helpful for testing and learning about system resource control, especially Linux cgroups (control groups) that allow resource restrictions on processes. 

## Table of Contents
1. [Concepts Used in the Program](#concepts-used-in-the-program)
2. [Using brox](#using-brox)
3. [Code Breakdown](#code-breakdown)

---

## Concepts Used in the Program

### 1. Cgroups
Cgroups (control groups) are a Linux feature used to limit, account, and isolate the resource usage (like memory or CPU) of a collection of processes. In this program, we create a cgroup to limit how much memory a process can use.

### 2. Memory Management
The program demonstrates memory allocation and management using `malloc` and `calloc`, which are standard functions in C for dynamic memory allocation. This is essential when handling large amounts of memory or when the memory requirements are unknown at compile time.

### 3. Privilege Dropping
The program drops privileges to a non-root user (the "nobody" user) after setting up the memory limits, improving security by reducing the permissions of the running process.

### 4. Structs and Typedefs
The program defines a `struct` named `MemBlk`, which is used to manage memory blocks efficiently. It also demonstrates using `typedef` to create custom data types.

---

## Using brox

### Help Command

To get a quick overview of how to use the program, you can run:

```bash
./brox help
```

### Running a Program with a Memory Limit

To run a program with a memory limit, use:

```bash
./brox run <program_command> <memory_in_MB> <debug_mode>
```

For example:
```bash
./brox run "stress --vm 1 --vm-bytes 512M --timeout 10s" 100 y
```

In this example, `brox` will limit the memory of `stress` to 100MB. The final "y" enables debug mode, which prints additional details during execution.

### Stress Testing with Memory Allocation

To perform a memory stress test, where `brox` continuously allocates memory:

```bash
./brox stress <memory_in_MB> <debug_mode>
```

For example:
```bash
./brox stress 500 y
```

This will stress-test by allocating up to 500MB.

---

## Code Breakdown

This section explains the code in `brox.c` line-by-line for a clear understanding.

### 1. Importing Libraries
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>
```

- **stdio.h**: Standard library for input and output functions like `printf`.
- **stdlib.h**: Contains general utilities like `malloc` for dynamic memory allocation and `exit`.
- **string.h**: Used for manipulating strings.
- **unistd.h**: Provides access to POSIX operating system API functions.
- **errno.h**: Defines macros for reporting and retrieving error conditions.
- **sys/stat.h**: Used for working with file statistics, including directory creation.
- **time.h**: Provides functions to get the current time.
- **sys/types.h**: Defines data types used in system calls.
- **pwd.h**: Used to retrieve user account information, such as `getpwnam`.

### 2. Defining the `MemBlk` Struct
```c
typedef struct {
    void *ptr;
    size_t sz;
    size_t used_sz;
} MemBlk;
```
- This defines a structure to represent a block of memory.
- **ptr**: Pointer to the allocated memory.
- **sz**: Total size of the memory block.
- **used_sz**: Size currently used within the memory block.

### 3. `h()` Function - Help Text
The `h()` function provides help information for the user.

### 4. `mb_create()` Function - Memory Block Creation
```c
MemBlk *mb_create(size_t sz) {
    ...
}
```
- **Allocates memory** for a new `MemBlk` struct and assigns `sz` as the size.

### 5. `mb_free()` Function - Freeing Memory
This function frees up memory allocated by `mb_create()`.

### 6. `dbg_log()` Function - Debug Logging
```c
void dbg_log(const char *msg, int dbg) {
    if (dbg) printf("[DBG] %s\n", msg);
}
```
Logs debug messages if `dbg` mode is enabled.

### 7. `drop_privs()` Function - Dropping Privileges
```c
int drop_privs(int dbg) {
    struct passwd *pw = getpwnam("nobody");
    ...
}
```
- This function retrieves the "nobody" user and switches to that user to enhance security.

### 8. `run_prog_with_cgroup()` Function - Running a Command in Cgroup
```c
int run_prog_with_cgroup(const char *cmd, size_t mem_limit, int dbg) {
    ...
}
```
- This function creates a cgroup and sets a memory limit on it.

### 9. `stress_test()` Function - Memory Stress Testing
```c
void stress_test(size_t tot_mem, int dbg) {
    ...
}
```
- Continuously allocates memory until the specified limit is reached, for testing.

### 10. `main()` Function - Entry Point
The `main()` function parses command-line arguments and handles program execution logic.

---
