#define MAX_CALL 100 // characters in call request string
#define MAX_COMMAND 50 //for allocating space for input
#define MAX_PENDING 1000 //number of processes that can wait for return
#define NUMBER_OF_QUEUES 100 //How many wait queues you have in sync
#define MAX_RESP 20 // total characters in response buffer
// define the debugfs path name directory and file
// full path name will be /sys/kernel/debug/getpid/call
char dir_name[] = "kernelsync";
char file_name[] = "call";

typedef struct syncOp{
	char * name;
	int one;
	int two;
} syncOp;

typedef struct caller {	
	pid_t pid;
	int returnValue;
	struct caller * prev;
	struct caller * next;
} caller;
