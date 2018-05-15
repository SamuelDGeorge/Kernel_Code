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
#include <linux/mm.h>


#include "log_fault.h" /* used by both kernel module and user program 
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
int VMA_index;
int initialized;
long fault_number;
unsigned long fault_count[MAX_VMA];
struct mm_struct * memory_struct;
struct vm_operations_struct vmas[MAX_VMA];
struct vm_operations_struct vmas_original[MAX_VMA];
struct vm_area_struct * vma_pointers[MAX_VMA];
pid_t current_pid;
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

static void log_fault(struct vm_area_struct * vma, struct vm_fault * vmf, int vma_number, u64 time) {
	char real_page_frame[MAX_CALL];	
	
	//printk(KERN_DEBUG "{log_fault}	Fault Number: %lu\n", fault_number);
	//printk(KERN_DEBUG "{log_fault}	VMA Fault Number: %lu\n", fault_count[vma_number]);
	
	if(vma != NULL && vmf != NULL) {	
		

		//printk(KERN_DEBUG "{log_fault}	vm_area_struct address: %p\n",vma);
				
		//printk(KERN_DEBUG "{log_fault}	Virtual page address: %lu\n",((unsigned long)(vmf->virtual_address))>>12);
		

		//printk(KERN_DEBUG "{log_fault}	Page Offset in VMA: %lu\n",vmf->pgoff);

		if(vmf->page != NULL) {
			sprintf(real_page_frame,"%lu",page_to_pfn(vmf->page));			
			//printk(KERN_DEBUG "{log_fault}	Real Page Frame: %lu\n",page_to_pfn(vmf->page));
		} else {
			sprintf(real_page_frame,"No_Page");	
			//printk(KERN_DEBUG "{log_fault}	Real Page Frame is NULL\n");
		}

		//printk(KERN_DEBUG "{log_fault}	Fault Resolved in: %llu\n\n", time);

		trace_printk(KERN_DEBUG "{log_fault}	%lu	%lu	%p	%lu	%lu	%s	%llu\n", fault_number,fault_count[vma_number],vma,((unsigned long)(vmf->virtual_address))>>12,vmf->pgoff,real_page_frame,time);
	}
}

static int find_matching_vma(struct vm_area_struct * vma) {
	int index = 0;
	while (index < VMA_index) {
		if ((vma_pointers[index] == vma) ) {
			
			return index;
		}
		index++;
	}
	return -1;
}

static int my_fault(struct vm_area_struct *vma,struct vm_fault *vmf) {
	int vma_number = 0;
	int return_number = 0;
	u64 start_time_stamp,end_time_stamp,time;
	
	//find the vma that matches
	vma_number = find_matching_vma(vma);
	
	if(vma_number == -1) {
		return VM_FAULT_SIGBUS;
	}

	//take time before handling fault	
	start_time_stamp = (u64) ktime_to_us(ktime_get());
	
	//call the original fault handler
	return_number = vmas_original[vma_number].fault(vma,vmf);
	
	//find how long it took
	end_time_stamp = (u64) ktime_to_us(ktime_get());
	
	time = end_time_stamp-start_time_stamp;

	fault_count[vma_number]++;
	fault_number++;
	
	log_fault(vma,vmf,vma_number,time);
	
	return return_number;
}
static void copy_and_redirect_vma(struct vm_area_struct * vma) {
	
	//save the origianl pointer
	vma_pointers[VMA_index] = vma;	

	//copy operation structure into local memory
	memcpy(&vmas[VMA_index], vma->vm_ops, sizeof(vmas[VMA_index]));

	//make another copy to save original fault pointer
	memcpy(&vmas_original[VMA_index], vma->vm_ops, sizeof(vmas[VMA_index]));

	//change local fault copy to point to my own function
	vmas[VMA_index].fault = my_fault;

	//point vma to your local copy
	vma->vm_ops = &vmas[VMA_index];
	
}

static int update_fault_pointers(struct task_struct * caller) {
	struct vm_area_struct * current_vma = memory_struct->mmap;
		
	//a new task has been called. Clear old list	
	VMA_index = 0;

	while (current_vma != NULL && VMA_index < MAX_VMA) {
		if(current_vma->vm_ops != NULL && current_vma->vm_ops->fault != NULL) {
			copy_and_redirect_vma(current_vma);			
			fault_count[VMA_index] = 0;			
			VMA_index++; 		
		}
		current_vma = current_vma->vm_next;
	}

	trace_printk(KERN_DEBUG "{start_log_fault}	Fault logging started for process with pid %i\n", current_pid);
	trace_printk(KERN_DEBUG "{log_fault}	Fault_Number	VMA_Fault_Number	Area_Address	Virtual_Address	Page_Offset	Real_Page_Frame	Time\n");
	fault_number = 0;	
	return 0;
}

static ssize_t log_fault_call(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
  int rc;
  char callbuf[MAX_CALL];  // local (kernel) space to store call string
  struct task_struct * current_task;
  struct rw_semaphore * semaphore;
  
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

  if(strcmp(callbuf, "log_faults") != 0) {
	printk(KERN_DEBUG "illegal call, log_faults only valid call.\n");
	return -1;
  }
  
  if(initialized == 1 && find_get_pid(current_pid) != NULL) {
	printk(KERN_DEBUG "Module in use! Call back later!\n");
	return -1;	
  }

  initialized = 1;
  current_task = current;
  current_pid = task_pid_nr(current);
  memory_struct = get_task_mm(current_task);
  semaphore = &memory_struct->mmap_sem;

  down_read(semaphore);
  rc = update_fault_pointers(current_task);  
  up_read(semaphore);

  if (rc != 0) {
	printk(KERN_DEBUG "failed to update pointers!\n");
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

static ssize_t log_fault_return(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
  return 0;
} 

// Defines the functions in this module that are executed
// for user read() and write() calls to the debugfs file
static const struct file_operations my_fops = {
        .read = log_fault_return,
        .write = log_fault_call,
};

/* This function is called when the module is loaded into the kernel
 * with insmod.  It creates the directory and file in the debugfs
 * file system that will be used for communication between programs
 * in user space and the kernel module.
 */

static int __init log_fault_init(void)
{

  /* create an in-memory directory to hold the file */

  dir = debugfs_create_dir(dir_name, NULL);
  if (dir == NULL) {
    printk(KERN_DEBUG "Log_Fault: error creating %s directory\n", dir_name);
     return -ENODEV;
  }

  /* create the in-memory file used for communication;
   * make the permission read+write by "world"
   */

  file = debugfs_create_file(file_name, 0666, dir, &file_value, &my_fops);
  if (file == NULL) {
    printk(KERN_DEBUG "Log_Fault: error creating %s file\n", file_name);
     return -ENODEV;
  }

  printk(KERN_DEBUG "Log_Fault: created new debugfs directory and file\n");

  printk(KERN_DEBUG "Log_Fault has been added to kernel\n");

  initialized = 0;

  return 0;
}

/* This function is called when the module is removed from the kernel
 * with rmmod.  It cleans up by deleting the directory and file and
 * freeing any memory still allocated.
 */

static void __exit log_fault_exit(void)
{
  debugfs_remove(file);
  debugfs_remove(dir);
  
}

/* Declarations required in building a module */

module_init(log_fault_init);
module_exit(log_fault_exit);
MODULE_LICENSE("GPL");
