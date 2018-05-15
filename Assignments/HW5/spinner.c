#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define MAX_TIME  30  //run time in seconds

  unsigned long count = 0;
  pid_t my_pid;
  time_t start, elapsed;

  my_pid = getpid();

  start = time(NULL);
  elapsed = 0;
  while (elapsed < MAX_TIME) {
    crypt("This is my lazy password", "A1");
    count++;
    elapsed = time(NULL) - start;
  }
  fprintf(stdout, "PID %d Elapsed time %ld iterations %ld\n", my_pid, elapsed, count);
