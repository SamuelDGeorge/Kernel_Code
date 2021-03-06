/*Samuel George, sdgeorge
I have Neither given nor recieved unauthorized aid on this assignment.
*/
/*
 * A simple user space program to illustrate calling an
 * emulated "system call" in programming assignments in
 * COMP 530H.  It opens the debugfs file used for calling
 * the getpid kernel module, requests the pid of the calling
 * process, and outputs the result.  It also outputs the 
 * result from the regular Linux getpid() system call so the
 * two results can be compared.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <crypt.h>
#include <sys/mman.h>
#include <fcntl.h>


#define MAX_CALL 100
#define MAX_RESP 100
#define MAX_PROCESSES 25
#define MAX_TIME 10

#include "log_fault.h" /* used by both kernel module and user program */

void do_syscall(char *call_string);  // does the call emulation

// variables shared between main() and the do_syscall() function
int fp;
char the_file[256] = "/sys/kernel/debug/";
char call_buf[MAX_CALL];  /* no call string can be longer */
char resp_buf[MAX_RESP];  /* no response strig can be longer */
int fdin;
unsigned long c = 0;
char *src;
struct stat statbuf;
int i, j;
int max_idx;

void map(void) {	
 /* open the input file */
 if ((fdin = open ("BigFile", O_RDONLY)) < 0)
   errx (-1, "can't open BigFile for reading");
 /* find size of input file */
 if (fstat (fdin, &statbuf) < 0)
   errx (-1, "fstat error");
 /* mmap the input file */
 if ((src = mmap (0, statbuf.st_size, PROT_READ, 
            MAP_SHARED, fdin, 0)) == (caddr_t) -1)
   errx (-1, "mmap error for input");
}

void test(void) {

 /* Random access to locations in the mapped file */
 fprintf(stdout, "Reading %lu bytes from mapped file\n", statbuf.st_size);
 max_idx = statbuf.st_size - 2;
 for (i = 0; i < statbuf.st_size; i++)
     {
       j = random() % max_idx;
       c += (*(src+j)) >> 2;
     }
 fprintf(stdout, "Read %d bytes, sum %lu\n", i-1, c);

}

void main (int argc, char* argv[])
{
  char inputstring[MAX_CALL]; 

  /* Build the complete file path name and open the file */

  strcat(the_file, dir_name);
  strcat(the_file, "/");
  strcat(the_file, file_name);

  if ((fp = open (the_file, O_RDWR)) == -1) {
      fprintf (stderr, "error opening %s\n", the_file);
      exit (-1);
  }

  map();

  sprintf(inputstring,"log_faults"); 
  do_syscall(inputstring);

  test();
					
  close (fp);
} /* end main() */

/* 
 * A function to actually emulate making a system call by
 * writing the request to the debugfs file and then reading
 * the response.  It encapsulates the semantics of a regular
 * system call in that the calling process is blocked until
 * both the request (write) and response (read) have been
 * completed by the kernel module.
 *  
 * The input string should be properly formatted for the
 * call string expected by the kernel module using the
 * specified debugfs path (this function does no error
 * checking of input).
 */ 


void do_syscall(char *call_string)
{
  int rc;

  strcpy(call_buf, call_string);

  rc = write(fp, call_buf, strlen(call_buf) + 1);
  if (rc == -1) {
     fprintf (stderr, "Module may be in use. Check kernel log.\n");
     fflush(stderr);
     exit(1);
  }

}

