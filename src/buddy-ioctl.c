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