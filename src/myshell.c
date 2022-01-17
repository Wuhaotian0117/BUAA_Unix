
/*
contract email: htwu1397624915@163.com 
myshell.c 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <ctype.h>

#define OUTFILE O_WRONLY|O_CREAT|O_TRUNC
#define OUTFILE_A O_WRONLY|O_CREAT|O_APPEND
#define PERM 0666

void myinit();
void parseline();
void execute();
void cmd_argvs(char *instr, char *argv[]);
void sys_err(const char *str);

typedef struct cmd{
	char *argv[10];
	int flag;
	int infd;
	int outfd;
} command;

char line[256] = {0};
command cmd1,cmd2;
char hiscmd[30][256];
int num = 0;
pid_t pid;
int status;
int redirectflag = 0;
int pipeflag = 0;
int pwdflag = 0;
int hisflag = 0;
int fd;
int pfd[2];
	
int main() {
	
	while(1) {
        printf("myshell$ ");
		myinit();
		fgets(line, 256, stdin);
		strcpy(hiscmd[num++], line);
		
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';
		}
		
		if (strcmp(line, "exit") == 0) {
			printf("Closing shell...\n");
			break;
		}
		parseline();
		execute();
	}
	return 0;
}

void myinit() {
	redirectflag = 0;
	pipeflag = 0;
	pwdflag = 0;
	hisflag = 0;
}

void parseline() {
	int i = 0;
	
	if (strstr(line, "history") != NULL) {
		hisflag = 1;
		return;
	}
	if (strstr(line, ">>") != NULL) {
			redirectflag = 3;
			return;
	}
	if (strncmp(line, "cd", 2) == 0) {
		pwdflag = 1;
		return;
	}
	for(i = 0; i < strlen(line); i++) {
		if (line[i] == '|') {
			pipeflag = 1;
			break;
		}
		if(line[i] == '>') {
			redirectflag = 1;
			break;
		}
		if (line[i] == '<') {
			redirectflag = 2;
			break;
		}
	}
}


void cmd_argvs(char *instr, char *argv[]) {
	
	int i = 0;
	argv[i] = strtok(instr, " ");
	while(argv[i])
	{
		i++;
		argv[i] = strtok(NULL," ");
	}
	argv[i] = NULL;
	
}
void execute() {
	if (hisflag != 0) {
		char *number = strtok(line, "history ");
		int total = 0, i = 0;
		
		for(i = 0; number[i] != '\0'; i++) {
			if (isdigit(number[i])) {
				int ob = number[i] - '0';
				total = total * 10 + ob;
			}
			else {
				printf("error: the index for history is wrong!\n");
				return;
			}
		}
                if (total >= num) {
                   printf("error: the index for history is wrong!\n");
                   return;
                }
		printf("%s", hiscmd[total]);
		return;
	}
	if (redirectflag != 0) {
		switch(redirectflag) {
			case 1 : {
				char *outstr = strtok(line, ">");
				char *src = strtok(NULL, ">");
				char *instr = strtok(src, " ");
				
				cmd_argvs(outstr, cmd1.argv);
				if((pid = fork()) < 0)
				{
					sys_err("fork error");
				}
				else if (pid == 0) {
					if((cmd1.outfd = open(instr,OUTFILE,PERM)) < 0)
					{
						sys_err("open failed");
					}
					dup2(cmd1.outfd,STDOUT_FILENO);
					close(cmd1.outfd);
					execvp(cmd1.argv[0],cmd1.argv);
					sys_err("excvp error");
				}
				else {
					memset(line, 0, 256);
					redirectflag = 0;
					waitpid(pid,&status,0);
				}
				break;
			}
			case 2 : {
				char *instr = strtok(line, "<");
				char *src = strtok(NULL, "<");
				char *outstr = strtok(src, " ");
				
				cmd_argvs(instr, cmd1.argv);
				if((pid = fork()) < 0)
				{
					sys_err("fork error");
				}
				else if(pid == 0)
				{
					if((cmd1.infd = open(instr,O_RDONLY)) < 0)
					{
						sys_err("open failed");
					}
					dup2(cmd1.infd,STDIN_FILENO);
					close(cmd1.infd);
					execvp(cmd1.argv[0],cmd1.argv);
					sys_err("execvp error");
				}
				else
				{
					memset(line, 0, 256);
					redirectflag = 0;
					waitpid(pid,&status,0);
				}
				break;
			}
			case 3 : {
				char *outstr = strtok(line, ">>");
				char *src = strtok(NULL, ">>");
				char *instr = strtok(src, " ");
				
				cmd_argvs(outstr, cmd1.argv);
				if((pid = fork()) < 0)
				{
					sys_err("fork error");
				}
				else if (pid == 0) {
					if ((cmd1.outfd = open(instr,OUTFILE_A,PERM)) < 0) {
						sys_err("open failed");
					}
					dup2(cmd1.outfd,STDOUT_FILENO);
					close(cmd1.outfd);
					execvp(cmd1.argv[0],cmd1.argv);
					sys_err("excvp error");
				}
				else {
					memset(line, 0, 256);
					redirectflag = 0;
					waitpid(pid,&status,0);
				}
				break;
			}
		}
	}
	else if (pipeflag != 0) {
		char *outstr = strtok(line, "|");
		char *instr = strtok(NULL,"|");
		cmd_argvs(outstr, cmd1.argv);
		cmd_argvs(instr, cmd2.argv);
		if((pid = fork()) < 0) {
			sys_err("fork error");
		}
		else if (pid == 0) {
			if (pipe(pfd) < 0) {
				sys_err("pipe error");
			}
			pid = fork();
			switch(pid) {
				case -1 : 
				    sys_err("fork error");
				case 0 : 
				    close(pfd[0]);
				    dup2(pfd[1],STDOUT_FILENO);
				    close(pfd[1]);
				    execvp(cmd1.argv[0],cmd1.argv);
				    sys_err("execvp error");
				    exit(-1);
				default : 
				    close(pfd[1]);
				    dup2(pfd[0],STDIN_FILENO);
				    close(pfd[0]);
				    execvp(cmd2.argv[0],cmd2.argv);
				    sys_err("execvp error");
				    exit(-1);
			}
		}
		else {
			memset(line, 0, 256);
			waitpid(pid,&status,0);
			pipeflag = 0;
		}
	}
	else if (pwdflag != 0) {
		char *path = strtok(line, "cd ");
		chdir(path);
	}
	else {
		cmd_argvs(line, cmd1.argv);
		if ((pid = fork()) < 0) {
			sys_err("fork error");
		}
		else if (pid == 0) {
			execvp(cmd1.argv[0],cmd1.argv);
			sys_err("execvp error");
		}
		else {
			waitpid(pid,&status,0);
		}
	}
}

void sys_err(const char *str)
{
	perror(str);
	exit(-1);
}
