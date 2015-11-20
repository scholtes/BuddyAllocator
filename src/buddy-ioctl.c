/* Author: Garrett Scholtes
 * Date:   2015-11-18
 *
 * buddy-ioctl.c - User space functions to wrap around ioctl calls.
 * Modeled after ioctl_07.c -- this does not contain the main function
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "buddy-dev.h"


// Request a block of memory of size size bytes from the memory manager whose handle is mem.
// Returns an integer which is a reference to the block (or a negative number on failure).
int get_mem(int mem, int size) {

    struct get_mem_struct params = {
        .mem = mem,
        .size = size
    };

    ioctl(mem, IOCTL_GET_MEM, (void *)(&params));

    return params.return_val;
}

// Free the block of memory referenced as ref from the memory manager whose handle is mem.
// Returns 0 on success and -1 on error
int free_mem(int mem, int ref) {

    struct free_mem_struct params = {
        .mem = mem,
        .ref = ref
    };

    ioctl(mem, IOCTL_FREE_MEM, (void *)(&params));

    return params.return_val;
}

// Writes the contents of the buffer buf to the memory block ref of the memory manager whose handle is mem.
// Stops writing when the first 0 is encountered in buf.
// Returns the number of bytes written or a negative number on error.
int write_mem(int mem, int ref, char *buf) {

    struct write_mem_struct params = {
        .mem = mem,
        .ref = ref,
        .buf = buf
    };

    ioctl(mem, IOCTL_WRITE_MEM, (void *)(&params));

    return params.return_val;
}

// Reads size bytes from the memory block ref of memory manager mem and returns the result in buf.
// The return value of read_mem is the number of bytes read or a negative number on error.
int read_mem(int mem, int ref, char *buf, int size) {

    struct read_mem_struct params = {
        .mem = mem,
        .ref = ref,
        .buf = buf,
        .size = size
    };

    ioctl(mem, IOCTL_READ_MEM, (void *)(&params));

    return params.return_val;
}