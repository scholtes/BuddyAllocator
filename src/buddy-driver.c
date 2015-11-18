/* Author: Garrett Scholtes
 * Date:   2015-11-18
 *
 * buddy-driver.c - Device driver for the very simple buddy allocator.
 * Modeled after driver-07.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "buddy-dev.h"
#define DEVICE_NAME "mem_dev"
#define BUDDY_BLOCK_SIZE (1<<12)
#define BUDDY_NUM_BLOCKS (1<<12)