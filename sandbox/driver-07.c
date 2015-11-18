/*
 *  chardev.c - Create an input/output character device
 *  http://linux.die.net/lkmpg/x892.html
 *
 *  Note: if an attempt is made to use get_user or put_user to transfer a 
 *  value that does not fit one of the specific sizes, the result is usually 
 *  a strange message from the compiler, such as "conversion to non-scalar 
 *  type requested." In such cases, 'copy_to_user' or 'copy_from_user' must 
 *  be used.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for get_user and put_user */

#include "chardev.h"
#define DEVICE_NAME "char_dev"
#define BUF_LEN 80

/* Is device open?  Prevents concurent access into the same device */
static int Device_Open = 0;

/* The message the device will give when asked */
static char Message[BUF_LEN];

/* 
 * How far did the process reading the message get?
 * Useful if the message is larger than the size of the
 * buffer we get to fill in device_read. 
 */
static char *Message_Ptr;

/* Called whenever a process attempts to open the device file */
static int open(struct inode *inode, struct file *file) {
   printk(KERN_INFO "open(%p)\n", file);

   /* Do not talk to two processes at the same time */
   if (Device_Open) return -EBUSY; else Device_Open++;

   /* Initialize the message */
   Message_Ptr = Message;
   return 0;
}

static int release(struct inode *inode, struct file *file) {
   printk(KERN_INFO "release(%p,%p)\n", inode, file);
   
   /* Clear Device_open - ready for the next caller */
   Device_Open--;
   return 0;
}

/* Called when a user process attempts to read from the the device   */
static ssize_t read(struct file *file,   /* see include/linux/fs.h   */
		    char *buffer,        /* buffer to be             *
				          * filled with data         */
		    size_t length,       /* length of the buffer     */
		    loff_t *offset) {
   /* Number of bytes actually written to the buffer */
   int bytes_read = 0;
   printk(KERN_INFO "read(%p,%p,%d)\n", file, buffer, length);
   
   /* Return 0 if at end of file  */
   if (*Message_Ptr == 0) return 0;
   
   /* Put the data into the buffer */
   while (length && *Message_Ptr) {
      
      /* Because the buffer is in the user data segment, not the kernel data 
       * segment, assignment does not work.  Instead, put_user is used to
       * copy data from the kernel data segment to the user data segment. */
      put_user(*(Message_Ptr++), buffer++);
      length--;
      bytes_read++;
   }
   
   printk(KERN_INFO "Read %d bytes, %d left\n", bytes_read, length);
   
   /* Read functions must return the number of bytes inserted into the buffer */
   return bytes_read;
}

/* Called when a user process attempts to write to the device file. */
static ssize_t write(struct file *file,
		     const char *buffer, 
		     size_t length, 
		     loff_t *offset) {
   int i;
   
   printk(KERN_INFO "write(%p,%s,%d)", file, buffer, length);
   
   for (i = 0; i < length && i < BUF_LEN; i++) get_user(Message[i], buffer+i);
   Message_Ptr = Message;
   
   /* The number of input characters used is returned  */
   return i;
}

/* Called when a process tries to do an ioctl on the device file. 
   There are two parameters in addition to the file struct: the number 
   of the ioctl called and the parameter given to the ioctl function.

   If the ioctl is write or read/write, output from the function is 
   returned to the calling process  */
long ioctl(struct file *file,
	   unsigned int ioctl_num,	/* number and param for ioctl */
	   unsigned long ioctl_param) {
   int i;
   char *temp;
   char ch;
   
   /* Switch according to the ioctl called */
   switch (ioctl_num) {
   case IOCTL_SET_MSG:
      /* Receive a pointer to a message (in user space) and set that
         to be the device's message.  Get the parameter given to 
         ioctl by the process. */
      temp = (char*)ioctl_param;
      
      /* Find the length of the message */
      get_user(ch, temp);
      for (i = 0; ch && i < BUF_LEN; i++, temp++) get_user(ch, temp);
      
      write(file, (char*)ioctl_param, i, 0);
      break;
      
   case IOCTL_GET_MSG:
      /* Give the current message to the calling process - the parameter 
         is a pointer whose space must be filled. */
      i = read(file, (char*)ioctl_param, 99, 0);
      printk(KERN_INFO "ioctl_get_msg: %s i=%d\n", (char*)ioctl_param,i);
      
      /* Put a zero at the end of the buffer for proper string terminated */
      put_user('\0', (char*)ioctl_param + i);
      break;
      
   case IOCTL_GET_NTH_BYTE:
      /* This ioctl has input (ioctl_param) and output (the return value) */
      return Message[ioctl_param];
      break;
   }
   
   return 0;
}

/* Module Declarations */

/* This structure holds the functions to be called when a process acts
   on the device.  Since a pointer to this structure is kept in the devices 
   table, it can't be local to init_module.  NULL is for unimplemented 
   functions. */
struct file_operations Fops = {
   .read = read,
   .write = write,
   .unlocked_ioctl = ioctl,
   .open = open,
   .release = release
};

/* Initialize the module - Register the character device */
int init_module(void) {
   int ret_val;
   /* Register the character device */
   ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
   if (ret_val < 0) {
      printk(KERN_ALERT "%s failed with %d\n",
	     "Sorry, registering the character device ", ret_val);
      return ret_val;
   }
   
   printk(KERN_INFO "%s The major device number is %d.\n",
	  "Registration is a success", MAJOR_NUM);
   
   return 0;
}

/* Cleanup - unregister the appropriate file from /proc  */
void cleanup_module(void) { unregister_chrdev(MAJOR_NUM, DEVICE_NAME);  }

MODULE_LICENSE("GPL");
