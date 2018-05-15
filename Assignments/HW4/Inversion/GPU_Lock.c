/*Samuel George, sdgeorge
I have Neither given nor recieved unauthorized aid on this assignment.
*/
/*
 *Implements wait queues in the kernel
 */ 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/rtmutex.h>

#include "GPU_Locks_kernel.h" /* used by both kernel module and user program 
                     * to define shared parameters including the
                     * debugfs directory and file used for emulating
                     * a system call
                     */

/* The following two variables are global state shared between
 * the "call" and "return" functions.  They need to be protected
 * from re-entry caused by kernel preemption.
 */
/* The call_task variable is used to ensure that the result is
 * returned only to the process that made the call.  Only one
 * result can be pending for return at a time (any call entry 
 * while the variable is non-NULL is rejected).
 */

int file_value;
static struct rt_mutex CE_Lock,EE_Lock;
static pid_t CE_Holder,EE_Holder;
struct dentry *dir, *file;  // used to set up debugfs file name

/* This function emulates the handling of a system call by
 * accessing the call string from the user program, executing
 * the requested function and preparing a response.
 *
 * This function is executed when a user program does a write()
 * to the debugfs file used for emulating a system call.  The
 * buf parameter points to a user space buffer, and count is a
 * maximum size of the buffer content.
 *
 * The user space program is blocked at the write() call until
 * this function returns.
 */

static int ce_lock(void) {
	rt_mutex_lock(&CE_Lock);
	CE_Holder = task_pid_nr(current);
	return 0;
}

static int ce_unlock(void) {
	if (CE_Holder != task_pid_nr(current)) {
		return -1;	
	}
	rt_mutex_unlock(&CE_Lock);
	return 0;
}

static int ee_lock(void) {
	rt_mutex_lock(&EE_Lock);
	EE_Holder = task_pid_nr(current);
	return 0;
}

static int ee_unlock(void) {
	if (EE_Holder != task_pid_nr(current)) {
		return -1;	
	}
	rt_mutex_unlock(&EE_Lock);
	return 0;
}

//Execute the desired command
static int execute_command(char * call) {
	if(strcmp(call,"CE_Lock") == 0) {
		return ce_lock();
	} else if (strcmp(call,"CE_UnLock") == 0) {
		return ce_unlock();
	} else if (strcmp(call,"EE_Lock") == 0) {
		return ee_lock();
	} else if (strcmp(call,"EE_UnLock") == 0) {
		return ee_unlock();
	} else {
		return -1;
	}
	
}

static ssize_t GPU_Lock_call(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
  int rc;
  char callbuf[MAX_CALL];  // local (kernel) space to store call string

  
  /* the user's write() call should not include a count that exceeds
   * the size of the module's buffer for the call string.
   */

  if(count >= MAX_CALL)
    return -EINVAL;  // return the invalid error code

  //This gets returned to the file system which then returns to the user. 
  //predefined error that a read or write can give.

  /* The preempt_disable() and preempt_enable() functions are used in the
   * kernel for preventing preemption.  They are used here to protect
   * state held in the call_task and respbuf variables
   */
  
  preempt_disable();  // prevents re-entry possible if one process 
                      // preempts another and it also calls this module
                      //stops the scheduler from pre-empting the program calling you
  //EAGAIN means I have an error, but you can try again later. 

  
  /* Use the kernel function to copy from user space to kernel space.
   */

  rc = copy_from_user(callbuf, buf, count);

  if (rc == -1) {	
	printk(KERN_DEBUG "Copy from user buffer failed\n");
	preempt_enable();
	return -1;
  }

  
  rc = execute_command(callbuf);

  //operation failed
  if (rc == -1) {    		
	printk(KERN_DEBUG "Command failed\n");	
	preempt_enable();
	return -1;
  }  

  preempt_enable();
  
  *ppos = 0;  /* reset the offset to zero */
  return 0;  /* write() calls return the number of bytes */
  //all write calls must return count
  //force the position back to the start of the file
}

/* This function emulates the return from a system call by returning
 * the response to the user as a character string.  It is executed 
 * when the user program does a read() to the debugfs file used for 
 * emulating a system call.  The buf parameter points to a user space 
 * buffer, and count is a maximum size of the buffer space. 
 * 
 * The user space program is blocked at the read() call until this 
 * function returns.
 */

static ssize_t GPU_Lock_return(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
  return 0;
} 

// Defines the functions in this module that are executed
// for user read() and write() calls to the debugfs file
static const struct file_operations my_fops = {
        .read = GPU_Lock_return,
        .write = GPU_Lock_call,
};

/* This function is called when the module is loaded into the kernel
 * with insmod.  It creates the directory and file in the debugfs
 * file system that will be used for communication between programs
 * in user space and the kernel module.
 */

static int __init GPU_Lock_module_init(void)
{

  /* create an in-memory directory to hold the file */

  dir = debugfs_create_dir(dir_name, NULL);
  if (dir == NULL) {
    printk(KERN_DEBUG "GPU_Lock: error creating %s directory\n", dir_name);
     return -ENODEV;
  }

  /* create the in-memory file used for communication;
   * make the permission read+write by "world"
   */

  file = debugfs_create_file(file_name, 0666, dir, &file_value, &my_fops);
  if (file == NULL) {
    printk(KERN_DEBUG "GPU_Lock: error creating %s file\n", file_name);
     return -ENODEV;
  }

  printk(KERN_DEBUG "GPU_Lock: created new debugfs directory and file\n");

  rt_mutex_init(&CE_Lock);
  rt_mutex_init(&EE_Lock);

  printk(KERN_DEBUG "Locks Initialized\n");

  return 0;
}

/* This function is called when the module is removed from the kernel
 * with rmmod.  It cleans up by deleting the directory and file and
 * freeing any memory still allocated.
 */

static void __exit GPU_Lock_module_exit(void)
{
  debugfs_remove(file);
  debugfs_remove(dir);
  
}

/* Declarations required in building a module */

module_init(GPU_Lock_module_init);
module_exit(GPU_Lock_module_exit);
MODULE_LICENSE("GPL");
