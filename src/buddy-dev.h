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



// Request to allocate a block of memory
// Last arameter get casted to:
//     [int *, int *]
#define IOCTL_GET_MEM _IOWR(MAJOR_NUM, 0, void **)


// Request to free memory
// Last arameter get casted to:
//     [int *, int *]
#define IOCTL_FREE_MEM _IOWR(MAJOR_NUM, 1, void **)


// Request to write to memory
// Last arameter get casted to:
//     [int *, int *, char *]
#define IOCTL_WRITE_MEM _IOwR(MAJOR_NUM, 2, void **)


// Request to read from memory
// Last arameter get casted to:
//     [int *, int *, char *, int *]
#define IOCTL_READ_MEM _IOWR(MAJOR_NUM, 3, void **)



#endif