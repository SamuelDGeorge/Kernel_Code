#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

  int fdin;
  unsigned long c = 0;
  char *src;
  struct stat statbuf;
  int i, j;
  int max_idx;

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

 /* Random access to locations in the mapped file */
 fprintf(stdout, "Reading %lu bytes from mapped file\n", statbuf.st_size);
 max_idx = statbuf.st_size - 2;
 for (i = 0; i < statbuf.st_size; i++)
     {
       j = random() % max_idx;
       c += (*(src+j)) >> 2;
     }
 fprintf(stdout, "Read %d bytes, sum %lu\n", i-1, c);
