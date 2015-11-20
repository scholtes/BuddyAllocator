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
#include <linux/string.h> // memset, strlen

#include "buddy-dev.h"
#define DEVICE_NAME "mem_dev"

MODULE_LICENSE("GPL");

// Is device open?  Prevents concurent access into the same device
static int Device_Open = 0;
// The actual block of memory to touch and play with
static char *memory;

/// ------------------------ BUDDY ALLOCATOR LOGIC ------------------------- ///

// A struct type keep track of our blocks and how they are fragmented.
// A better way would be to keep an array to represent the binary tree.  Would use less space
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

// Given an address, get the block pointer for the
// block that contains the memory at that address.
// Returns NULL if ref is out of range
struct block_node *__get_block_from_address(int ref) {
    int block_idx;
    int nth_bit;
    int n;
    struct block_node *current_node;

    if(ref < 0 || MEM_SIZE <= ref) return NULL;

    // The binary representation of the block index (block_idx) from right to left,
    // starting at the BUDDY_BLOCK_DEPTH bit, will tell us whether the node we are
    // looking for is in the right or left half of the tree
    block_idx = ref / BUDDY_BLOCK_SIZE;
    current_node = buddy_root;
    for(n = BUDDY_BLOCK_DEPTH-1; n >= 0; n--) {
        // If the current node is not a leaf, then we already found our block pointer
        if(current_node->state != PARENT) {
            break;
        }
        nth_bit = (block_idx >> n) & 1;
        // If 1, go right; else go left
        if(nth_bit) {
            current_node = current_node->right_child;
        } else {
            current_node = current_node->left_child;
        }
    }

    return current_node;
}

// Given a memory size, give a reference to that block.
// Returns a -1 if the request could not be satisfied
int get_mem(int size, struct block_node *block, int available) {
    int ref;
    ref = 0;

    // Case: user has requested more memory than is available
    if(available < size) {
        return -1;
    }
    // Case: the current block is free and has enough memory
    if(block->state == FREE) {
        // Inner case: the current block is too big so we split it into buddies
        if(size <= (available>>1) && BUDDY_BLOCK_SIZE <= (available>>1)) {
            __split_block(block);
            return get_mem(size, block->left_child, available>>1);
        }
        // Inner case: the current block is already the proper size
        block->state = ALLOCATED;
        return 0;
    }
    // Case: the current block is allocated
    if(block->state == ALLOCATED) {
        return -1;
    }
    // Case: the current block node is not a leaf
    // Step 1: scan the left tree for open space and return if we found some
    ref = get_mem(size, block->left_child, available>>1);
    if(ref >= 0) {
        return ref;
    }
    // Step 2: scan the right tree for open space.  Add an offset if we found
    //         space, otherwise just return a -1
    ref = get_mem(size, block->right_child, available>>1);
    if(ref < 0) {
        return -1;
    }
    ref += (available>>1);
    return ref;

}

// Frees memory.  0 on success, -1 on failure
int free_mem(int ref) {
    struct block_node *block;
    block = __get_block_from_address(ref);

    if(block == NULL || block->state != ALLOCATED) {
        return -1;
    }

    __free_and_merge(block);

    return 0;
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

static ssize_t read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    copy_to_user(buffer, memory + (long)offset, length);

    return length;
}

static ssize_t write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    copy_from_user(memory + (long)offset, buffer, length);

    return length;
}

/// -------------- Some more buddy allocator wrapper functions ------------- ///

// Writes to memory.  Num bytes written on success, -1 on failure
int write_mem(struct file *file, int ref, char *buf) {
    struct block_node *block1;
    struct block_node *block2;
    int size;
    long rf;

    size = strlen(buf);

    block1 = __get_block_from_address(ref);
    block2 = __get_block_from_address(ref + size -1);

    // Sanity check -- if the blocks are NULL or if they aren't the same,
    // then there is an error
    rf = ref;
    if(block1 && block1 == block2) {
        return (int)write(file, buf, size, (loff_t *)rf);
    }

    return -1;
}

// Reads from memory.  Num bytes read on success, -1 on failure
int read_mem(struct file *file, int ref, char *buf, int size) {
    struct block_node *block1;
    struct block_node *block2;
    long rf;

    block1 = __get_block_from_address(ref);
    block2 = __get_block_from_address(ref + size -1);

    // Sanity check -- if the blocks are NULL or if they aren't the same,
    // then there is an error
    rf = ref;
    if(block1 && block1 == block2) {
        return (int)read(file, buf, size, (loff_t *)rf);
    }

    return -1;
}

/// ------------------------------------------------------------------------ ///

long ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
    switch(ioctl_num) {
    case IOCTL_GET_MEM:
        printk("    get_mem(...)\n");
        ((struct get_mem_struct *)ioctl_param)->return_val = get_mem(
            ((struct get_mem_struct *)ioctl_param)->size,
            buddy_root,
            MEM_SIZE
        );
        break;
    case IOCTL_FREE_MEM:
        printk("    free_mem(...)\n");
        ((struct free_mem_struct *)ioctl_param)->return_val = free_mem(
            ((struct free_mem_struct *)ioctl_param)->ref
        );
        break;

    case IOCTL_WRITE_MEM:
        printk("    write_mem(...)\n");
        ((struct write_mem_struct *)ioctl_param)->return_val = write_mem(
            file,
            ((struct write_mem_struct *)ioctl_param)->ref,
            ((struct write_mem_struct *)ioctl_param)->buf
        );
        break;

    case IOCTL_READ_MEM:
        printk("    read_mem(...)\n");
        ((struct read_mem_struct *)ioctl_param)->return_val = read_mem(
            file,
            ((struct read_mem_struct *)ioctl_param)->ref,
            ((struct read_mem_struct *)ioctl_param)->buf,
            ((struct read_mem_struct *)ioctl_param)->size
        );
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
    memset(memory, 0, MEM_SIZE);

    buddy_root = kmalloc(sizeof(struct block_node), GFP_KERNEL);
    buddy_root->state = FREE;
    buddy_root->parent = NULL;
    buddy_root->left_child = NULL;
    buddy_root->right_child = NULL;

    printk("Success! Major number = %d\n", MAJOR_NUM);

    return 0;
}

void cleanup_module(void) {
    printk("Buddy Allocator cleaning up...\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    kfree(memory);
    free_tree(buddy_root);
}