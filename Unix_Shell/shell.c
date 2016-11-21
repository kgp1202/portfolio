#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

//Background Circular Queue
typedef struct BackgroundQueue{
	int queueSize;
	int* queue;
	int qHead;
	int qTail;
	
	int (*insert)(struct BackgroundQueue*, int);
	int (*delete)(struct BackgroundQueue*, int);
} BackgroundQueue;

BackgroundQueue* q;

//Insert number to circular queue
int insert(BackgroundQueue* self, int inserted_pid){	
	int added_position = (self->qTail + 1) % self->queueSize;
	if(self->qHead == added_position){
		printf("Background Queue is full\n");
		return -1;
	}

	self->queue[added_position] = inserted_pid;
	self->qTail = added_position;
	return 0;
}

//Delete number to circular queue
int delete(BackgroundQueue* self, int deleted_pid){
	int curr = (self->qHead + 1) % self->queueSize;
	int next = (curr + 1) % self->queueSize;
	int currValue = self->queue[curr];
	int temp;
	self->queue[curr] = -1;
	while(currValue != deleted_pid && curr != self->qHead){
		temp = currValue;
		currValue = self->queue[next];
		self->queue[next] = temp;

		curr = next;
		next = (curr + 1) % self->queueSize; 
	}
	
	if(currValue == deleted_pid){
		self->qHead = (self->qHead + 1) % self->queueSize;
		return 0;
	}else if(next != self->qHead) {
		printf("There is no pid\n");
		return -1;
	}

	printf("ERROR");
	return -1;
}

//Initate function for circualr queue
BackgroundQueue* BackgroundQueueInit(int queueSize){
	BackgroundQueue* self = malloc(sizeof(BackgroundQueue));

	self->queueSize = queueSize + 1;
	self->queue = (int*)malloc(queueSize);
	self->qHead = 0;
	self->qTail = 0;
	
	self->insert = &insert;
	self->delete = &delete;

	return self;
}

//Change to Lowercase
char* ctol(char* input){
	char* changed = (char*)malloc(strlen(input));
	for(int i = 0; i < strlen(input); i++){
		changed[i] = input[i] | 0x20;
	}
	return changed;
}

int shell(int argc, char* argv[]){
	//Change to lowerCase
	for(int i = 0; i < argc; i++)
		argv[i] = ctol(argv[i]);
	
	if(strcmp(argv[0], "exit") == 0){
		return -2;
	}
	else if(strcmp(argv[0], "cd") == 0){
		chdir(argv[1]);
		return 0;
	}
	//kill process manually
	else if(strcmp(argv[0], "killone") == 0){
		int deleted_pid = atoi(argv[1]);
		if(q->delete(q, deleted_pid) == 0){
			printf("[%d] is killed", deleted_pid);
			kill(deleted_pid, SIGKILL);
			waitpid(deleted_pid, NULL, 0);
			return 0;
		}else {
			printf("queue delete error\n");
			return -1;
		}
	}
	
	int backgroundFlag = 0;
	int pipeFlag = 0;
	int pipeBound;
	for(int i = 0; i < argc; i++){
		if(strcmp(argv[i], "|")== 0){
			pipeFlag = 1;
			pipeBound = i;
			argv[i] = NULL;
		}
		else if(strcmp(argv[i], "&") == 0){
			backgroundFlag = 1;
			argv[i] = NULL;
		} 
	}
	argv[argc] = NULL;

	pid_t id = fork();
	if(id == 0){	//child process
		if(pipeFlag == 1){
			int p[2];
			pipe(p);

			pid_t pipeId = fork();
			if(pipeId == 0){
				close(p[1]);
				dup2(p[0], 0);
				execvp(argv[0], argv);
				return -1;
			}else {
				close(p[0]);
				dup2(p[1], 1);
				execvp(argv[pipeBound + 1], &argv[pipeBound + 1]); 
				return -1;
			}
		}else {
			execvp(argv[0], argv);
			return -1;
		}
	}else {				//parent process
		if(backgroundFlag == 1){
			q->insert(q, id);
		}else {
			waitpid(id, NULL, 0);
		}
		return 0;
	}
	
	return -1;
}

void autoKillChild(int signo){
	pid_t pid = wait(NULL);
	q->delete(q, pid);
	return ;
}

int main(){	
	//This make catch SIGCHLD and kill zombie process
	signal(SIGCHLD, autoKillChild);

	char input[256];
	char* argv[256];
	int argc = 0;

	q = BackgroundQueueInit(5);

	while(1){
		fgets(input, 256, stdin);
		argc = 0;

		char temp[256];
		int tempCount = 0;
		sscanf(input, "%s", temp);

		//Divide input to char**
		for(int i = 0; i < strlen(input); i++){
			if(input[i] == ' ' || input[i] == '\n'){
				temp[tempCount] = '\0';
				argv[argc] = (char*)malloc(tempCount);
				strcpy(argv[argc], temp);
				argc++;
				tempCount = 0;
			}else {
				temp[tempCount++] = input[i];
			}
		}
		
		//To do shell
		switch(shell(argc, argv)){
			case -2:
				printf("BYE!!");
				return 0;
			case -1:
				printf("ERROR!!");
				return 0;
			case 0:
				continue;
		}
	}
}
