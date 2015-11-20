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

// My own tests.  Mainly to verify buddy allocator logic for
// getting and freeing memory.
// Designed only to test with BUDDY_BLOCK_SIZE = BUDDY_NUM_BLOCKS = 16 
void my_test() {
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

   printf("\n------------- Running my tests -------------\n");
   my_test();

   return 0;
}