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

MODULE_LICENSE("GPL");


static int open(struct inode *inode, struct file *file) { return 0; }

static int release(struct inode *inode, struct file *file) { return 0; }

static ssize_t read(struct file *file, char *buffer, size_t length, loff_t *offset) { return 0; }

static ssize_t write(struct file *file, const char *buffer, size_t length, loff_t *offset) { return 0; }

long ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) { return 0; }


struct file_operations Fops = {
   .read = read,
   .write = write,
   .unlocked_ioctl = ioctl,
   .open = open,
   .release = release
};

int init_module(void) {
    int ret_val;

    printk("Buddy Allocator loading...\n");

    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
    if(ret_val < 0) {
        printk(KERN_ALERT "***Could not load buddy allocator***\n");
        return ret_val;
    }

    printk("Success! Major number = %d\n", MAJOR_NUM);

    return 0;
}

void cleanup_module(void) {
    printk("Buddy Allocator cleaning up...\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}