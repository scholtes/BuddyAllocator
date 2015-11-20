/* Author: Garrett Scholtes
 * Date:   2015-11-18
 *
 * buddy-dev.h - Header with ioctl definitions, used both by kernel AND userspace.
 * Modeled after chardev.h
 */

#ifndef BUDDYDEV_H
#define BUDDYDEV_H

#include <linux/ioctl.h>

// Our statically chosen major device number
#define MAJOR_NUM 150



// Define structs used to pass parameters
struct get_mem_struct {
    int mem;
    int size;

    int return_val;
};

struct free_mem_struct {
    int mem;
    int ref;

    int return_val;
};

struct write_mem_struct {
    int mem;
    int ref;
    char *buf;
    int size;

    int return_val;
};

struct read_mem_struct {
    int mem;
    int ref;
    char *buf;
    int size;

    int return_val;
};



// Request to allocate a block of memory
// Last parameter get casted to:
//     struct get_mem_struct *
#define IOCTL_GET_MEM _IOR(MAJOR_NUM, 0, void *)


// Request to free memory
// Last parameter get casted to:
//     struct free_mem_struct *
#define IOCTL_FREE_MEM _IOR(MAJOR_NUM, 1, void *)


// Request to write to memory
// Last parameter get casted to:
//     struct write_mem_struct *
#define IOCTL_WRITE_MEM _IOR(MAJOR_NUM, 2, void *)


// Request to read from memory
// Last parameter get casted to:
//     struct read_mem_struct *
#define IOCTL_READ_MEM _IOR(MAJOR_NUM, 3, void *)



#endif