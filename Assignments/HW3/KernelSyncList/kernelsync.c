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

#include "kernelsync.h" /* used by both kernel module and user program 
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

char * op; //for collecting commands
int file_value;
static wait_queue_head_t queues[NUMBER_OF_QUEUES];  //for holding wait queues
static int initialized[NUMBER_OF_QUEUES]; //is it initialized or not
static int waitingExclusive[NUMBER_OF_QUEUES]; //easy way to track number waiting
static int waitingNonExclusive[NUMBER_OF_QUEUES]; 
static struct caller pendingCallers[MAX_PENDING + 1]; //array for matching callers to returns
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


//finds the next empty space
static int find_next_empty(void) {
	int i = 0;
	for (i = 0; i < MAX_PENDING; i++) {
		if (pendingCallers[i].pid == 0) {
			return i;
		}
	}
	return -1;
}

//find the return that matches your PID
static int find_matching(pid_t pid) {
	int i = 0;
	for(i = 0; i < MAX_PENDING + 1; i++) {
		if(pendingCallers[i].pid == pid) {
			return i;
		}
	}
	return -1;
}

//remove from list
static int remove_from_list(pid_t pid) {
	int i = 0;
	for(i = 0; i < MAX_PENDING + 1; i++) {
		if(pendingCallers[i].pid == pid) {
			pendingCallers[i].pid = 0;
			pendingCallers[i].returnValue = -1;
			return 0;
		}
	}
	return -1;
}

//code for doing create
static int event_create(int event) {
	//create if not initialized. If it is, error. 	
	if (initialized[event] == 0) {
		init_waitqueue_head(&queues[event]);
		initialized[event] = 1;
		waitingExclusive[event] = 0;
		waitingNonExclusive[event] = 0;
		return 0;
	}	
	return -1;
}

//code for doing wait
static int event_wait(int event, int exclusive) {
	DEFINE_WAIT(wait);	
	//check the queue is initialized	
	if (initialized[event] == 0) {
		return -1;
	}

	if (exclusive == 0) {
		//increment waiting non-exclusive		
		waitingNonExclusive[event] = waitingNonExclusive[event] + 1;		

		//do wait
		add_wait_queue(&queues[event],&wait);
		prepare_to_wait(&queues[event],&wait, TASK_INTERRUPTIBLE);				
		schedule();
	} else if (exclusive == 1) {
		//increment waiting exclusive	
		waitingExclusive[event] = waitingExclusive[event] + 1;	
		
		//do wait	
		add_wait_queue_exclusive(&queues[event],&wait);
		prepare_to_wait_exclusive(&queues[event],&wait, TASK_INTERRUPTIBLE);	
		schedule();
	} else {
		return -1;	
	}
	finish_wait(&queues[event],&wait);
	return 0;
	
}

//code for doing signal
static int event_signal(int event) {
	//check the queue is initialized	
	if (initialized[event] == 0) {
		return -1;
	}
	
	//make sure you aren't signaling an empty queue
	if(waitingExclusive[event] == 0 && waitingNonExclusive[event] == 0) {
		return -1;
	}

	//wake up
	wake_up(&queues[event]);

	//all nonExclusive awoke
	waitingNonExclusive[event] = 0;

	//decrement waitingExclusive if there were any waiting exclusivly 
	if (waitingExclusive[event] > 0) {
		waitingExclusive[event] = waitingExclusive[event] - 1;
	}

	return 0;
}

//does destroy
static int event_destroy(int event) {	
	//make sure the queue exist	
	if(initialized[event] == 0)  {
		return -1;
	}

        //if jobs are pending wake them up, otherwise just destory queue
	if(waitingExclusive[event] != 0 || waitingNonExclusive[event] != 0) {
		wake_up_all(&queues[event]);
	}

	//destory queue
	initialized[event] = 0;
	waitingExclusive[event] = 0;
	waitingNonExclusive[event] = 0;
			
	return 0;
}

//parses the string passed in from user buffer
static int parseInput(char * call, struct syncOp * currentOperation) {
	int one = 100;
	int two = 100;	
	
	//make room for operation string
	op = kmalloc(MAX_CALL,GFP_ATOMIC);	

  	if (op == NULL) {  // always test if allocation failed
   	    preempt_enable(); 
 	    return -ENOSPC;
	}

        //collect input
	sscanf(call, "%s %i[ %i]",op,&one,&two);

        //find if it is in correct format and build 
	//the operation
	if (strcmp(op, "event_create") == 0) {
		if (one >=0 && one <= 99) {
			(*currentOperation).name = op;
			(*currentOperation).one = one;
			return 0;
		} else {
			return -1;		
		}
	} else if (strcmp(op, "event_wait") == 0) {
		if (one >=0 && one <= 99 && (two == 1 || two == 0)) {
			(*currentOperation).name = op;
			(*currentOperation).one = one;
			(*currentOperation).two = two;
			return 0;
		} else {
			return -1;		
		}
	} else if (strcmp(op, "event_signal") == 0) {
		if (one >=0 && one <= 99) {
			(*currentOperation).name = op;
			(*currentOperation).one = one;
			return 0;
		} else {
			return -1;		
		}
	}  else if (strcmp(op, "event_destroy") == 0) {
		if (one >=0 && one <= 99) {
			(*currentOperation).name = op;
			(*currentOperation).one = one;
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

//does operation
static int performOperation(struct syncOp * currentOperation, int callerSpot) {
	//perform the operation	
	if (strcmp((*currentOperation).name,"event_create") == 0) {
		return event_create((*currentOperation).one);
	} else if (strcmp((*currentOperation).name,"event_wait") == 0 && callerSpot != MAX_PENDING) {
		//don't let a wait be the last function called, otherwise everyone is hung		
		return event_wait((*currentOperation).one,(*currentOperation).two);
	} else if (strcmp((*currentOperation).name,"event_signal") == 0) {
		return event_signal((*currentOperation).one);
	} else if (strcmp((*currentOperation).name,"event_destroy") == 0) {
		return event_destroy((*currentOperation).one);
	} 	
	return -1;
}


static ssize_t kernelsync_call(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
  int rc;
  struct syncOp * currentOperation;
  int callerSpot;
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

  if (rc == -1 || callbuf[MAX_CALL - 1] != '\0') {	
	printk(KERN_DEBUG "Copy from user buffer failed\n");
	preempt_enable();
	return count;
  }
  
  //allocate space for the operation structure
  currentOperation = kmalloc(sizeof(syncOp),GFP_ATOMIC);
	
  if (currentOperation == NULL) {  // always test if allocation failed
     preempt_enable(); 
     return -ENOSPC;
  }

  callerSpot = find_next_empty();

  if (callerSpot == -1) {
	printk(KERN_DEBUG "Max Sized Reached, using last spot for Call\n");	
	callerSpot = MAX_PENDING;	
  }
  
  pendingCallers[callerSpot].pid = task_pid_nr(current);

  //take the call buffer and create syncOp structure from it
  rc = parseInput(callbuf,currentOperation);

  //invalid input
  if (rc == -1) {	
	printk(KERN_DEBUG "Invalid Syntax\n");
	pendingCallers[callerSpot].returnValue = -1;		
	preempt_enable();
	return count;	
  }

  

  //assume operation will succeed and set its return value
  pendingCallers[callerSpot].returnValue = currentOperation->one;

  //perform the desired operation
  rc = performOperation(currentOperation,callerSpot);
  

  //operation failed
  //find which one and change return
  if (rc == -1) {
        callerSpot = find_matching(task_pid_nr(current));
	if(callerSpot == MAX_PENDING) {
		printk(KERN_DEBUG "Last Operation Can't be a Wait\n");
	}
	pendingCallers[callerSpot].returnValue = -1;        	
	printk(KERN_DEBUG "Operation Failed\n");		
	preempt_enable();
	return count;
  }  


  //currentOperation no longer needed, free it
  kfree(currentOperation);


  printk(KERN_DEBUG "kernelsync: call %s will return from call.\n", callbuf);
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

static ssize_t kernelsync_return(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
  int rc; 
  char * respbuf;
  int returnItem;
  int currentReturn;

  preempt_disable(); // protect static variables
  
  //allocate room for response
  respbuf = kmalloc(MAX_RESP,GFP_ATOMIC);

  if (respbuf == NULL) {  // always test if allocation failed
     preempt_enable(); 
     return -ENOSPC;
  }

  printk(KERN_DEBUG "Return call is going to find matching PID\n");
  
  //find response from your matching caller
  returnItem = find_matching(task_pid_nr(current));
  
  //write return
  if(returnItem == -1) {
	currentReturn = -1;
  } else {
	currentReturn = pendingCallers[returnItem].returnValue;
  }
 
  strcpy(respbuf,"");
  sprintf(respbuf,"%i",currentReturn);

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

  //If you found a return spot, clear it for another process
  if (returnItem != -1) {
	remove_from_list(task_pid_nr(current));	
  }
  
  
 
  respbuf = NULL;

  preempt_enable(); // clear the disable flag

  *ppos = 0;  /* reset the offset to zero */
  return rc;  /* read() calls return the number of bytes */
} 

// Defines the functions in this module that are executed
// for user read() and write() calls to the debugfs file
static const struct file_operations my_fops = {
        .read = kernelsync_return,
        .write = kernelsync_call,
};

/* This function is called when the module is loaded into the kernel
 * with insmod.  It creates the directory and file in the debugfs
 * file system that will be used for communication between programs
 * in user space and the kernel module.
 */

static int __init kernelsync_module_init(void)
{

  /* create an in-memory directory to hold the file */

  dir = debugfs_create_dir(dir_name, NULL);
  if (dir == NULL) {
    printk(KERN_DEBUG "kernelsync: error creating %s directory\n", dir_name);
     return -ENODEV;
  }

  /* create the in-memory file used for communication;
   * make the permission read+write by "world"
   */

  file = debugfs_create_file(file_name, 0666, dir, &file_value, &my_fops);
  if (file == NULL) {
    printk(KERN_DEBUG "kernelsync: error creating %s file\n", file_name);
     return -ENODEV;
  }

  printk(KERN_DEBUG "kernelsync: created new debugfs directory and file\n");

  return 0;
}

/* This function is called when the module is removed from the kernel
 * with rmmod.  It cleans up by deleting the directory and file and
 * freeing any memory still allocated.
 */

static void __exit kernelsync_module_exit(void)
{
  debugfs_remove(file);
  debugfs_remove(dir);
  
}

/* Declarations required in building a module */

module_init(kernelsync_module_init);
module_exit(kernelsync_module_exit);
MODULE_LICENSE("GPL");
