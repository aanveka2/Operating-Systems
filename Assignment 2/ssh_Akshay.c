#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>  
#include <fcntl.h> 


#define BUFFER 100
#define LINE 100



char *history[100] ;
int historyLength = 0;
int historypid[100] ;


pid_t pid,pid1,pid2,kpid,cpid;
char *arg_array[100];
char *arg_array1[100];
char *arg_array2[100];
int num_args=0;

int Lineparse(char* buffer, char** arg_array);
int process_commandline(char** arg_array,int num_args, char** arg_array1, char** arg_array2, char * buffer);

void terminate_process(int sig) {
        signal(SIGINT,terminate_process);
        printf("\n");
		printf("ssh_550> ");
		fflush(stdout); 
}

void sigtstp_handler(int sig) {
    
	if(sig == SIGCHLD)
	{
		kill(pid, SIGKILL);
		fflush(stdout); 
	}
}

int main() 
{
    pid1=getpid();
    char commandline[LINE], buffer [100];
	pid2=getpgrp();
    signal(SIGINT, terminate_process);
	//signal(SIGCHLD, sigtstp_handler);

    tcsetpgrp(STDIN_FILENO, pid2);
    
    while (1) {
        printf("ssh 550> ");
        if(fgets(commandline, LINE, stdin) == 0)
		{
			fprintf(stderr, "Input Not Read");
			return 0;
		}
        strcpy(buffer,commandline);
        num_args = Lineparse(buffer,arg_array);
        if (!strncmp(arg_array[0], "exit",4)) {
			printf("Exit \n");
			waitpid(-1, NULL, WNOHANG);
			exit(0);
		}
        process_commandline(arg_array,num_args,arg_array1,arg_array2,buffer);
    } 
    return 0;
}


int Lineparse(char* buffer, char** arg_array) {
    char * delimiter;
    int num_args;
    buffer[strlen(buffer) - 1] = ' ';
    while ( * buffer && ( * buffer == ' '))
        buffer++;

    num_args = 0;
    while ((delimiter = strchr(buffer, ' '))) {
        arg_array[num_args++] = buffer;
        * delimiter = '\0';
        buffer = delimiter + 1;
        while ( * buffer && ( * buffer == ' '))
            buffer++;
    }
    arg_array[num_args] = NULL;
    return num_args;
}

int process_commandline(char** arg_array,int num_args, char** arg_array1, char** arg_array2, char * buffer) {
    int run_bg;
    pid_t pid;
      
    // checking special built functions
    if (arg_array[0] == NULL)
        return 0;
    else if (strncmp(arg_array[0], "exit",4)== 0)
        	exit(0);
    else if (strncmp(arg_array[0], "fg", 2) == 0) {
        	printf("foreground process %d \n",atoi(arg_array[1]));
        	int child_status;
       
        	if (waitpid((__pid_t) atoi(arg_array[1]), &child_status, 0) < 0)
        	{

            	perror("foreground process() error");
            	exit(EXIT_FAILURE);
            
        	}
        } 
        else if (strncmp(arg_array[0], "listjobs", 7) == 0) {

            printf("List of Background process:\n");
            for (int i=0;i < historyLength ;i++)
            {
				
			   printf("PID %d of command  %s \n", historypid[i],history[i]  );
            }
			
         }
			setpgid(0,0);
           	history[historyLength] = strdup(buffer);              
    	 	historypid[historyLength++] = getpid();
        
        
		 // fork for exec
        if ((pid = fork()) == 0)
        {
            cpid=getpid();
			if(strncmp(arg_array[0],"kill", 4)==0)
			{
				kpid=atoi(arg_array[1]);
				printf("Killed process with pid: %d \n",kpid);
				signal(SIGCHLD, sigtstp_handler);
				if(kill(kpid, SIGKILL)==-1)
				{
					perror("Kill error");
					return 1;
				}
				return 0;	
			}

    		//Redirection of Input and Output stream
    		int i=0,fd;
			for(i=0;i<num_args;i++)
			{
			if(strncmp(arg_array[i],"|", 1)==0)
			{
				int x=0;
				for(int j=0; j< num_args;j++)
				{
					
					if(strncmp(arg_array[j],"|",1)==0)
					{
						for(int k=0; k< j;k++)
						{ 
							memmove(&arg_array1[k],&arg_array[k],sizeof(char *));	//Split the array and move the left part of pipe
						}
						
						for(int l=j+1; l<=num_args;l++)
						{
							memmove(&arg_array2[x],&arg_array[l],sizeof(char *));	//Split the array and move the left part of pipe
							x++;
						}										
					}
				}
            
            for(int a=0; a< num_args;a++)
				{
					int fd1[2]; // File Descriptor
					if(pipe(fd1) == -1) 
					{
						perror("Error in creating a pipe");
						exit(1);
					}
					pid_t pipe_pid = fork();
					if (pipe_pid == -1) 
					{ 
						printf("Error to fork");
						return 1;
					}
					if(pipe_pid == 0) 
					{			
							close(1);//Close stdout
							dup(fd1[1]);//Duplicate write end
							close(fd1[0]);//Read end closed
							close(fd1[1]);//Write end closed
							if(execvp(arg_array1[0], arg_array1)< 0);  
							{
								printf("Error in Execution \n");      
								exit(1);
							}
					}
					else if(pipe_pid>0) 
					{	int k=x-1;
						
						close(0);//Close stdin
						dup(fd1[0]);//Duplicate read end
						close(fd1[0]);//Read end closed
						close(fd1[1]);//Write end closed
						for(int h=0;h<k;h++)//last command after '|' 
						{
							if(strcmp(arg_array2[h],"<")==0)
							{
								if (arg_array2[h+1] == NULL)
								{
									printf("No Arguments \n");
								}
								arg_array2[h] = NULL;
								fd = open(arg_array2[h+1], O_RDONLY);  // open a file descriptor to read
								if(dup2(fd,0)<0)
								perror("Error in Duplication");
								close(fd);
								
							}
							if(strcmp(arg_array2[h],">")==0)
							{
								if (arg_array2[h+1] == NULL)
								{
									printf("No Arguments  \n");
								}
								arg_array2[h] = NULL;
								fd = open(arg_array2[h+1], O_CREAT | O_TRUNC | O_WRONLY,0600);  //open a file descriptor to write 
								if(dup2(fd,1)== -1)
								{
									perror("Error in Duplication");
									close(fd);
								}
							}
						}
						if(execvp(arg_array2[0], arg_array2)==-1)
						{
							printf("Error in Execution \n");      
							exit(1);
						}
					}					
				}
				
			}

			else if(strncmp(arg_array[i],"<",1)==0)
			{
				if (arg_array[i+1] == NULL)
				{
                    printf("No Arguments\n");
				}
				arg_array[i] = NULL;
				fd = open(arg_array[i+1], O_RDONLY); // open a file descriptor to read
				if(dup2(fd,0)<0)
				perror("Error in Duplication");
				close(fd);
				
			}
			else if(strncmp(arg_array[i],">",1)==0)
			{
				if (arg_array[i+1] == NULL)
				{
                    printf("Not enough arguments\n");
				}
				arg_array[i] = NULL;
				fd = open(arg_array[i+1], O_CREAT | O_TRUNC | O_WRONLY,0600); // open a file descriptor to write
				if(dup2(fd,1)<0)
					perror("Error in Duplication");
				close(fd);
			}
          
        }
		if ((run_bg = ( * arg_array[num_args - 1] == '&')) != 0) {
			arg_array[--num_args] = NULL;
			setpgid(0,0);
			printf( "Process %d is in background mode \n",getpid());
	    }
		printf("last exec \n");
		if (execvp(arg_array[0], arg_array)==-1)
		{
			//printf("%s: Command not found.\n", arg_array[0]);
			exit(1);
		}
		exit(0);
	}
else
{
	 if (!run_bg) 
	 {
        
        int child_status;
        if (waitpid(pid, & child_status, 0) < 0)
        {
			fprintf(stderr, "Error in Running Foreground Process  \n");
			exit(EXIT_FAILURE);
		}
		return 1;
		}
}
}


	

	
