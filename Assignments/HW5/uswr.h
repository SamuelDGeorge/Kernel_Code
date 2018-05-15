#define MAX_CALL 100
#define MAX_RESP 100
#define MAX_PROCESSES 25
#define MAX_CONCURRENT 10
char syscall_location[] = "/sys/kernel/debug/";
char dir_name[] = "uswr";
char file_name[] = "call";


typedef struct callerWeight{
	pid_t pid;
	int weight;
	int is_turn;
} callerWeight;
