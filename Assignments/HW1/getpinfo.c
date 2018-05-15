/*Samuel George, sdgeorge
I have Neither given nor recieved unauthorized aid on this assignment.
*/
/*
 * Example kernel loadable module.  It illustrates the 
 * module infrastructure used in programming assignments
 * in the COMP 530H Lab.  The only function is to accept
 * an emulated "system call" to getpid from user space
 * and returns the character representation of the 
 * Linux process ID (pid) of the caller.
 */ 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "getpinfo.h" /* used by both kernel module and user program 
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

struct task_struct *call_task = NULL;
struct task_struct *parent = NULL;
struct mm_struct *memory = NULL;
char *respbuf;  // points to memory allocated to return the result

int file_value;
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

static void append_and_clear(char * full, char * toAdd) {
	strcat(full, toAdd);
        memset(toAdd,0,MAX_LINE);
}

static ssize_t getpinfo_call(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
  int rc;
 
  //loff_t is the position that the pointer is currently at in the file. 
  //always a null character at the end of a stirng.  
  //build all the responses into individual segments to eventually combine into a single buffer
  char callbuf[MAX_CALL];  // local (kernel) space to store call string
  char resp_buff[MAX_LINE]; // local (kernel) space for putting in lines to buffer.


  //declare local variables to hold fields
  pid_t cur_pid,par_pid;
  char * command;
  int state,flags,priority;
  int vm_area,vm_shared,vm_exec,vm_stack,vm_total;

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

  if (call_task != NULL) { // a different process is expecting a return
     preempt_enable();  // must be enabled before return
     return -EAGAIN;
  }

  //EAGAIN means I have an error, but you can try again later. 



  // allocate some kernel memory for the response
  //give it now, or give an error
  respbuf = kmalloc(MAX_RESP, GFP_ATOMIC);
  if (respbuf == NULL) {  // always test if allocation failed
     preempt_enable(); 
     return -ENOSPC;
  }

  //no space



  strcpy(respbuf,""); /* initialize buffer with null string */

  /* current is global for the kernel and contains a pointer to the
   * task_struct for the running process  
   */
  call_task = current;
  
  //get the parent task and associated memory manager for the task
  parent = (*call_task).real_parent;
  memory = get_task_mm(current);

  /* Use the kernel function to copy from user space to kernel space.
   */

  rc = copy_from_user(callbuf, buf, count);
  callbuf[MAX_CALL - 1] = '\0'; /* make sure it is a terminated string */

  if (strcmp(callbuf, "getpinfo") != 0) { // only valid call is "getpinfo"
      strcpy(respbuf, "Failed: invalid operation\n");
      printk(KERN_DEBUG "getpinfo: call %s will return %s\n", callbuf, respbuf);
      preempt_enable();
      return count;  /* write() calls return the number of bytes written */
  }

  //make sure things like prempt go in pairs. 
  sprintf(respbuf, "Success:\n"); // start forming a response in the buffer

  /* Use kernel functions for access to various needed fields 
   */
  //get access to all the task structure fields
  cur_pid = task_pid_nr(current);
  command  = kmalloc(MAX_LINE,GFP_ATOMIC);
  get_task_comm(command,current);
  par_pid = task_pid_nr(parent);
  state = (*current).state;
  flags = (*current).flags;
  priority = (*current).normal_prio;
  vm_area = (*memory).map_count;
  vm_shared = (*memory).shared_vm;
  vm_exec = (*memory).exec_vm;
  vm_stack = (*memory).stack_vm;
  vm_total = (*memory).total_vm;

  //Print all statments into a temporary buffer and combine
  sprintf(resp_buff, "Current PID %d\n", cur_pid);
  append_and_clear(respbuf,resp_buff);

  sprintf(resp_buff,"  command %s\n",command); 
  append_and_clear(respbuf,resp_buff);
  kfree(command);
  
  sprintf(resp_buff,"  parent PID %d\n",par_pid); 
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  state %i\n",state);   
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  flags 0x%x\n",flags); 
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  priority %i\n",priority);
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  VM areas %i\n",vm_area);
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  VM shared %i\n",vm_shared);
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  VM exec %i\n",vm_exec);
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  VM stack %i\n",vm_stack);
  append_and_clear(respbuf,resp_buff);
  
  sprintf(resp_buff,"  VM total %i\n",vm_total);
  append_and_clear(respbuf,resp_buff);


  /* Here the response has been generated and is ready for the user
   * program to access it by a read() call.
   */

  printk(KERN_DEBUG "getpinfo: call %s will return %s", callbuf, respbuf);
  preempt_enable();
  
  *ppos = 0;  /* reset the offset to zero */
  return count;  /* write() calls return the number of bytes */
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

static ssize_t getpinfo_return(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
  int rc; 

  preempt_disable(); // protect static variables

  if (current != call_task) { // return response only to the process making
                              // the getpid request
     preempt_enable();
     return 0;  // a return of zero on a read indicates no data returned
  }
  //make sure the call task is asking for the return.

  rc = strlen(respbuf) + 1; /* length includes string termination */

  /* return at most the user specified length with a string 
   * termination as the last byte.  Use the kernel function to copy
   * from kernel space to user space.
   */

  /* Use the kernel function to copy from kernel space to user space.
   */
  if (count < rc) { // user's buffer is smaller than response string
    respbuf[count - 1] = '\0'; // truncate response string
    rc = copy_to_user(userbuf, respbuf, count); // count is returned in rc
  }
  else 
    rc = copy_to_user(userbuf, respbuf, rc); // rc is unchanged

  kfree(respbuf); // free allocated kernel space

  respbuf = NULL;
  call_task = NULL; // response returned so another request can be done

  preempt_enable(); // clear the disable flag

  *ppos = 0;  /* reset the offset to zero */
  return rc;  /* read() calls return the number of bytes */
} 

// Defines the functions in this module that are executed
// for user read() and write() calls to the debugfs file
static const struct file_operations my_fops = {
        .read = getpinfo_return,
        .write = getpinfo_call,
};

/* This function is called when the module is loaded into the kernel
 * with insmod.  It creates the directory and file in the debugfs
 * file system that will be used for communication between programs
 * in user space and the kernel module.
 */

static int __init getpinfo_module_init(void)
{

  /* create an in-memory directory to hold the file */

  dir = debugfs_create_dir(dir_name, NULL);
  if (dir == NULL) {
    printk(KERN_DEBUG "getpinfo: error creating %s directory\n", dir_name);
     return -ENODEV;
  }

  /* create the in-memory file used for communication;
   * make the permission read+write by "world"
   */

  file = debugfs_create_file(file_name, 0666, dir, &file_value, &my_fops);
  if (file == NULL) {
    printk(KERN_DEBUG "getpinfo: error creating %s file\n", file_name);
     return -ENODEV;
  }

  printk(KERN_DEBUG "getpinfo: created new debugfs directory and file\n");

  return 0;
}

/* This function is called when the module is removed from the kernel
 * with rmmod.  It cleans up by deleting the directory and file and
 * freeing any memory still allocated.
 */

static void __exit getpinfo_module_exit(void)
{
  debugfs_remove(file);
  debugfs_remove(dir);
  if (respbuf != NULL)
     kfree(respbuf);
}

/* Declarations required in building a module */

module_init(getpinfo_module_init);
module_exit(getpinfo_module_exit);
MODULE_LICENSE("GPL");
