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
#include <linux/sched/rt.h>

#include "SchedCl.h"
#include "uswr.h" /* used by both kernel module and user program 
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
int initialized;
char * op;
struct callerWeight * callers[MAX_PROCESSES];
const struct sched_class *rt_class_p;
static struct sched_class uwrr_sched_class;
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

//used to track which process are using this module to schedule
static int add_to_process_list(struct callerWeight * caller) {
	int index = 0;
	
	while(index < 25) {
		//if it is empty put it here		
		if (callers[index] == NULL) {
			callers[index] = caller;
			return 0;
		}
		
		//if it is the same process calling again, change its weight
		if(callers[index]->pid == task_pid_nr(current)) {
			callers[index] = caller;
			return 0;
		}

		index++;
	}

	//process list is full, clear it and add new process. Never more than set max concurrent processes. 
	if (index == MAX_PROCESSES) {
		//clear first ten items in the list	
		for (index = 0; index < (MAX_PROCESSES - MAX_CONCURRENT); index++) {
			callers[index] = NULL;
		} 
		//put current entry at first position
		callers[0] = caller;
		return 0;
	
	}

	return -1;
}

static int find_task(void) {
	int index = 0;
	//check all the spots in the list of stored processes	
	while (index < MAX_PROCESSES) {
		if (callers[index] == NULL) {
			//do nothing
		} else if(callers[index]->pid == task_pid_nr(current)) {
			return index;
		}
		index++;
	}
	return -1;
}

static void task_tick_uwrr_default(void *rq, struct task_struct *p, int queued) {
	rt_class_p->task_tick(rq, p, queued);
}

static void task_tick_uwrr (void *rq, struct task_struct *p, int queued)
{
	u64 start_time_stamp;
	u64 end_time_stamp;
	int current_task;
	 
	
	current_task = find_task();
	
	//couldn't find you, just schedule normally	
	if(current_task == -1) {
		task_tick_uwrr_default(rq,p,queued);
		return;
	}
	
	//check if the task has started its quantum yet, if not, set it's time slice
	if (callers[current_task]->is_turn == 0) {
		callers[current_task]->is_turn = 1;
		p->rt.time_slice = 10 * callers[current_task]->weight + 1;
		start_time_stamp = (u64) ktime_to_us(ktime_get());
		printk(KERN_DEBUG "	{uswr}	[%i]	%i	start	%llu\n",task_pid_nr(current),callers[current_task]->weight,start_time_stamp);
	}

	//your quantum is about to expire, it is no longer your turn
	if(p->rt.time_slice == 1) {
		callers[current_task]->is_turn = 0;
	}	
	
	rt_class_p->task_tick(rq, p, queued);

	//your slice ended. Record when it ended
	if(callers[current_task]->is_turn == 0) {
		end_time_stamp = (u64) ktime_to_us(ktime_get());
		printk(KERN_DEBUG "	{uswr}	[%i]	%i	end	%llu\n",task_pid_nr(current),callers[current_task]->weight,end_time_stamp);
	}
	
	
	return;
}


//used to initialize the structure of the real time task
static int initialize_structure(void) {
    rt_class_p = current->sched_class;
    memcpy(&uwrr_sched_class, rt_class_p, sizeof(uwrr_sched_class));
    uwrr_sched_class.task_tick = task_tick_uwrr;
   
    printk(KERN_DEBUG "The Real-Time Scheduler Has been Replaced!\n");
    initialized = 1;
    return 0;
}

static int parseInput(char * call, struct callerWeight * caller) {
	int one = 100;

	//make room for operation string
	op = kmalloc(MAX_CALL,GFP_ATOMIC);	

  	if (op == NULL) {  // always test if allocation failed
   	    preempt_enable(); 
 	    return -ENOSPC;
	}

	//check if the caller has valid priority and scheduler
	if ((*current).rt_priority != 1) {
		printk(KERN_DEBUG "Task Calling does not have the appropriate priority.\n");		
		return -1;
	}

        //collect input
	sscanf(call, "%s %i",op,&one);

        //find if it is in correct format and build 
	//the structure to hold this tasks weight
	if (strcmp(op, "uwrr") == 0) {
		if (one >=1 && one <= 20) {
			caller->pid = task_pid_nr(current);
			caller->weight = one;
			return 0;
		} else {
			return -1;		
		}
	} else {
		return -1;		
	}
        //bad operation, failed
	return -1;
}

static ssize_t uswr_call(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
  int rc;
  struct callerWeight * currentCaller;
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

  //allocate space for the caller structure
  currentCaller = kmalloc(sizeof(callerWeight),GFP_ATOMIC);

  //failed to allocate
  if(currentCaller == NULL) {
	printk(KERN_DEBUG "Unable to get kernel space\n");
	preempt_enable();
	return -1;
  }
  
  rc = parseInput(callbuf,currentCaller);

  //operation failed
  if (rc == -1) {    		
	printk(KERN_DEBUG "Invalid Input\n");
	preempt_enable();
	return -1;
  }  

  //Store the caller int the list of process
  rc = add_to_process_list(currentCaller);

  if (rc == -1) {    		
	printk(KERN_DEBUG "Too Many Concurrent Processes. Exiting.\n");
	preempt_enable();
	return -1;
  }  

  //call is valid, go ahead and do the work
  //Call if it is not initialized
  if(initialized == 0) {
	rc = initialize_structure();
  }

  if (rc == -1) {    		
	printk(KERN_DEBUG "Initialization Failed!\n");
	preempt_enable();
	return -1;
  }  
	
  //set the current classes scheduler to my scheduler class
  current->sched_class = &uwrr_sched_class;

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

static ssize_t uswr_return(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
  return 0;
} 

// Defines the functions in this module that are executed
// for user read() and write() calls to the debugfs file
static const struct file_operations my_fops = {
        .read = uswr_return,
        .write = uswr_call,
};

/* This function is called when the module is loaded into the kernel
 * with insmod.  It creates the directory and file in the debugfs
 * file system that will be used for communication between programs
 * in user space and the kernel module.
 */

static int __init uswr_init(void)
{

  /* create an in-memory directory to hold the file */

  dir = debugfs_create_dir(dir_name, NULL);
  if (dir == NULL) {
    printk(KERN_DEBUG "USWR: error creating %s directory\n", dir_name);
     return -ENODEV;
  }

  /* create the in-memory file used for communication;
   * make the permission read+write by "world"
   */

  file = debugfs_create_file(file_name, 0666, dir, &file_value, &my_fops);
  if (file == NULL) {
    printk(KERN_DEBUG "USWR: error creating %s file\n", file_name);
     return -ENODEV;
  }

  printk(KERN_DEBUG "USWR: created new debugfs directory and file\n");

  initialized = 0;

  printk(KERN_DEBUG "USWR has been added to kernel\n");

  return 0;
}

/* This function is called when the module is removed from the kernel
 * with rmmod.  It cleans up by deleting the directory and file and
 * freeing any memory still allocated.
 */

static void __exit uswr_exit(void)
{
  debugfs_remove(file);
  debugfs_remove(dir);
  
}

/* Declarations required in building a module */

module_init(uswr_init);
module_exit(uswr_exit);
MODULE_LICENSE("GPL");
