

#ifndef SBUSH_PROCESS_H
#define SBUSH_PROCESS_H
typedef struct Process{
	struct Process* next;
	char* path;
	char** argv;
	int argc;
	int fd_out; //initialize -1
	int fd_in;  //initialize -1
	int process_id; //initialize -1
}Process;

typedef struct Job{
 	Process * first_process; /*list of process in this job*/
	int isBackground; /* 1 if is, 0 if not background*/
}Job; 
int ProcessCommand (char* command);

#endif
