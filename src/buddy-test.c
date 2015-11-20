/* Author: Garrett Scholtes
 * Date:   2015-11-18
 *
 * buddy-test.c - A user space application to test the buddy allocator.
 * Modeled after ioctl_07.c -- this contains the main function
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "buddy-ioctl.c"

// Sample usage provided by Dr. Franco from lab 8 page
void franco_test() {
    int mem, ref;
    char buffer[4096];

    mem = open("/dev/mem_dev", 0);
    ref = get_mem(mem, 100);
    printf("reference = %d\n", ref);
    sprintf(buffer, "Hello buddy");
    write_mem(mem, ref, buffer);
    read_mem(mem, ref+3, buffer, 10);
    printf("buffer: %s\n", buffer); /* buffer: lo buddy */
    free_mem(mem, ref);
    close(mem);
}

// Internal fragmentation test.  After allocating just
// over half of the available memory, even a request of 1 byte 
// is expected to fail
void fragmentation_test() {
    int mem, ref;

    mem = open("/dev/mem_dev", 0);
    printf("Allocating just over half of space...\n");

    ref = get_mem(mem, (MEM_SIZE>>1) + 1);
    printf("-Expected: %d, Actual: %d\n", 0, ref); // Should start at beginning
    printf("Allocating 1 byte (should fail)...\n"); // Not possible because of fragmentation
    ref = get_mem(mem, 1);
    printf("-Expected: %d, Actual: %d\n", -1, ref);

    printf("Freeing lots of mem...\n");
    free_mem(mem, 0);
    printf("Allocating 1 byte (should pass)...\n"); // Should start at beginning
    ref = get_mem(mem, 1);
    printf("-Expected: %d, Actual: %d\n", 0, ref);

    free_mem(mem, 0);
    close(mem);
}

/* Miscellany tests.  Mainly to verify buddy allocator logic for
 getting and freeing memory.
 Designed only to test with BUDDY_BLOCK_SIZE = BUDDY_NUM_BLOCKS = 16 
 
 The first 6 get_mems, if working properly, should cause the buddies to
 follow this sequence:
 
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|_ _ _ _|_ _ _ _ _ _ _ _|    <- After asking for 4 blocks
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|_ _|_ _ _ _ _ _ _ _|    <- After asking for 2 blocks
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|X X|_ _ _ _ _ _ _ _|    <- After asking for 2 blocks
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|X X|X X X X|_ _ _ _|    <- After asking for 4 blocks
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|X X|X X X X|X|_|_ _|    <- After asking for 1 block
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|X X|X X X X|X|X|_ _|    <- After asking for 1 block

         FAILED REQUEST to ask for 4 more blocks
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|X X|_ _ _ _|X|X|_ _|    <- After free block index 8   
          _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
         |X X X X|X X|X X|X X X X|X|X|_ _|    <- After asking for 4 blocks
                                                 (this time it should succeed)
*/
void misc_test() {
    int mem;

    if(BUDDY_BLOCK_DEPTH != 4) {
        printf("    These tests were hardcoded for BUDDY_BLOCK_DEPTH = 4 only\n");
        return;
    }

    mem = open("/dev/mem_dev", 0);
    // Free and allocate a bunch of memory chunks

    printf("Expected: %d, Actual: %d\n", 0 * 16, get_mem(mem, 4 * 16));
    printf("Expected: %d, Actual: %d\n", 4 * 16, get_mem(mem, 2 * 16));
    printf("Expected: %d, Actual: %d\n", 6 * 16, get_mem(mem, 2 * 16));
    printf("Expected: %d, Actual: %d\n", 8 * 16, get_mem(mem, 4 * 16));
    printf("Expected: %d, Actual: %d\n", 12 * 16, get_mem(mem, 1 * 16));
    printf("Expected: %d, Actual: %d\n", 13 * 16, get_mem(mem, 1 * 16));
    printf("Expected: %d, Actual: %d\n", -1, get_mem(mem, 4 * 16));
    printf("Expected: %d, Actual: %d\n", 0, free_mem(mem, 8 * 16));
    printf("Expected: %d, Actual: %d\n", 8 * 16, get_mem(mem, 4 * 16));

    free_mem(mem, 0 * 16);
    free_mem(mem, 4 * 16);
    free_mem(mem, 6 * 16);
    free_mem(mem, 8 * 16);
    free_mem(mem, 12 * 16);
    free_mem(mem, 13 * 16);

    close(mem);
}

int main(int argc, const char **argv) {

   printf("-------- Running Dr. Franco's tests --------\n");
   franco_test();

   printf("\n-------- Running fragmentation test --------\n");
   fragmentation_test();

   printf("\n------------ Running misc tests ------------\n");
   misc_test();

   return 0;
}