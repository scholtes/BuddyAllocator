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

int main(int argc, const char **argv) {

   franco_test();

   return 0;
}