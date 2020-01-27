#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER 100
# define LINE 200


int Lineparse(char * buffer, char ** arg_array);
void process_commandline(char * commandline);
char *history[100] ;
int historyLength = 0;
int historypid[100] ;


int Lineparse(char * buffer, char ** arg_array) {
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

void process_commandline(char * commandline) {
    char * arg_array[BUFFER];
    char buffer[LINE];
    int run_bg;
    pid_t pid;

    
    strcpy(buffer, commandline);
    
    int num_args = Lineparse(buffer, arg_array);
    
    // checking special built functions
    if (arg_array[0] == NULL)
        return;
    if (!strcmp(arg_array[0], "exit"))
        exit(0);
    else if (strncmp(arg_array[0], "fg", 2) == 0) {
        printf("foreground process %d",  atoi(arg_array[1]));
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
                 printf("PID %d of command  %s", historypid[i],history[i]  );
            }
           
         }
            
        else {
        if ((run_bg = ( * arg_array[num_args - 1] == '&')) != 0) {
            arg_array[--num_args] = NULL;
        }
          history[historyLength] = strdup(commandline);              
          historypid[historyLength++] = pid;

    // fork for exec
        if ((pid = fork()) == 0)
        {
               if (execvp(arg_array[0], arg_array) < 0) 
            {
                printf("%s: Command not found.\n", arg_array[0]);
                exit(0);
            }
        }
        if (!run_bg) {
            printf("pid: %d \n", pid);
            int child_status;
            if (waitpid(pid, & child_status, 0) < 0)
            {
                fprintf(stderr, "Error in Running Foreground Process  \n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main() {
    char commandline[LINE];
    while (1) {
        printf("ssh 550> ");
        fgets(commandline, LINE, stdin);
        if (feof(stdin))
            exit(0);
        process_commandline(commandline);
    }
}
