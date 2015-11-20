/* Author: Garrett Scholtes
 * Date:   2015-11-18
 *
 * buddy-driver.c - Device driver for the very simple buddy allocator.
 * Modeled after driver-07.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h> // kmalloc, kfree
#include <asm/uaccess.h>

#include "buddy-dev.h"
#define DEVICE_NAME "mem_dev"

// TODO change these back to (1<<12) when done testing
#define BUDDY_BLOCK_SIZE (1<<4)
#define BUDDY_NUM_BLOCKS (1<<4)
#define MEM_SIZE (BUDDY_NUM_BLOCKS * BUDDY_BLOCK_SIZE)

MODULE_LICENSE("GPL");

// Is device open?  Prevents concurent access into the same device
static int Device_Open = 0;
// The actual block of memory to touch and play with
static char *memory;

/// ------------------------ BUDDY ALLOCATOR LOGIC ------------------------- ///

// A struct type keep track of our blocks and how they are fragmented
enum block_state {PARENT, ALLOCATED, FREE};
struct block_node {
    enum block_state state;
    struct block_node *parent;
    struct block_node *left_child;
    struct block_node *right_child;
};

// Here is the root of our tree
static struct block_node *buddy_root;

// Called on buddy_root when module is unloaded to free an entire tree
static void free_tree(struct block_node *block) {
    if(block->left_child) free_tree(block->left_child);
    if(block->right_child) free_tree(block->right_child);
    kfree(block);
}

// Given a block, splits it into two buddies
static void __split_block(struct block_node *block) {
    block->state = PARENT;

    block->left_child = kmalloc(sizeof(struct block_node), GFP_KERNEL);
    block->left_child->state = FREE;
    block->left_child->parent = block;
    block->left_child->left_child = NULL;
    block->left_child->right_child = NULL;

    block->right_child = kmalloc(sizeof(struct block_node), GFP_KERNEL);
    block->right_child->state = FREE;
    block->right_child->parent = block;
    block->right_child->left_child = NULL;
    block->right_child->right_child = NULL;
}

// Given a leaf node, make it free and attempt to merge it with it's buddy
static void __free_and_merge(struct block_node *block) {
    struct block_node *parent;

    block->state = FREE;
    parent = block->parent;

    if(parent && parent->left_child->state == FREE && parent->right_child->state == FREE) {
        // If the parent exists it is guaranteed to have
        // both a left and right child so this is valid.
        // Another invariant is that block==parent->left_child || block==parent->right_child
        kfree(parent->left_child);
        kfree(parent->right_child);
        parent->left_child = NULL;
        parent->right_child = NULL;
        __free_and_merge(parent);
    }
}

/// ------------------------------------------------------------------------ ///


static int open(struct inode *inode, struct file *file) {
    printk("----open(...)\n");

    // TODO: a more sound approach would be to use a read-write semaphore.  This
    // is just what driver-07.c does, and for now it will do.
    if(Device_Open) {
        return -EBUSY;
    } else {
        Device_Open++;
    }

    return 0;
}

static int release(struct inode *inode, struct file *file) {
    printk("----release(...)\n");

    Device_Open--;
    
    return 0;
}

static ssize_t read(struct file *file, char *buffer, size_t length, loff_t *offset) { return 0; }

static ssize_t write(struct file *file, const char *buffer, size_t length, loff_t *offset) { return 0; }

long ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {

    switch(ioctl_num) {
    case IOCTL_GET_MEM:
        printk("    get_mem(...)\n");
        // TOOD: FILL IN
        //((struct get_mem_struct *)ioctl_param)->return_val = 12;

        break;
    case IOCTL_FREE_MEM:
        printk("    free_mem(...)\n");
        // TOOD: FILL IN
        break;

    case IOCTL_WRITE_MEM:
        printk("    write_mem(...)\n");
        // TOOD: FILL IN
        break;

    case IOCTL_READ_MEM:
        printk("    read_mem(...)\n");
        // TOOD: FILL IN
        break;

    default:
        printk(KERN_ALERT "Invalid IOCTL switch %d!\n", ioctl_num);
        break;
    }

    return 0;
}


struct file_operations Fops = {
   .read = read,
   .write = write,
   .unlocked_ioctl = ioctl,
   .open = open,
   .release = release
};



void test_get_block_from_address(void) {
    __split_block(buddy_root);
    __split_block(buddy_root->left_child);
    __split_block(buddy_root->left_child->right_child);
    __split_block(buddy_root->right_child);
    __split_block(buddy_root->right_child->right_child);
    __split_block(buddy_root->right_child->right_child->left_child);

    __free_and_merge(buddy_root->left_child->right_child->left_child);
    __free_and_merge(buddy_root->right_child->right_child->left_child->left_child);
}




int init_module(void) {
    int ret_val;

    printk("Buddy Allocator loading...\n");

    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
    if(ret_val < 0) {
        printk(KERN_ALERT "***Could not load buddy allocator***\n");
        return ret_val;
    }

    // GFP flag so that we can sleep while not in use but also greedily grab memory
    memory = kmalloc(MEM_SIZE, GFP_KERNEL);
    buddy_root = kmalloc(sizeof(struct block_node), GFP_KERNEL);
    buddy_root->state = FREE;
    buddy_root->parent = NULL;
    buddy_root->left_child = NULL;
    buddy_root->right_child = NULL;
    test_get_block_from_address();

    printk("Success! Major number = %d\n", MAJOR_NUM);

    return 0;
}

void cleanup_module(void) {
    printk("Buddy Allocator cleaning up...\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    kfree(memory);
    free_tree(buddy_root);
}