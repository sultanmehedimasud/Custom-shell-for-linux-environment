#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>


void show_terminal() {
    printf("sh> ");
    fflush(stdout);  
}

char* input_read() {
    static char input[1000];  

    if (fgets(input, 1000, stdin) == NULL) {
        return 0;
    }

    input[strcspn(input, "\n")] = '\0';
    return input;
}

char** split_input(char *input) {
    static char *input_array[1000];
    char *temp;
    int i = 0;

    temp = strtok(input, " ");
    while (temp != NULL) {
        input_array[i++] = temp;
        temp = strtok(NULL, " ");
    }
    input_array[i] = NULL;
    return input_array;
}
struct File {
    char* file_name;
    int input_mode;
    int append_mode;
};

struct File check_file(char **input_array) {
    struct File file_detail;
    file_detail.file_name = NULL;
    file_detail.input_mode = 0;
    file_detail.append_mode = 0;
    int i = 0;
    while (input_array[i] != NULL) {
        if (strcmp(input_array[i], "<") == 0) {
            file_detail.file_name = input_array[i + 1];
            input_array[i] = NULL;
            file_detail.input_mode = 1;
            break;
        } else if (strcmp(input_array[i], ">") == 0) {
            file_detail.file_name = input_array[i + 1];
            input_array[i] = NULL;
            break;
        } else if (strcmp(input_array[i], ">>") == 0) {
            file_detail.file_name = input_array[i + 1];
            input_array[i] = NULL;
            file_detail.append_mode = 1;
            break;
        }
        i++;
    }
    return file_detail;
}


void execute_system_command(char *input) {
    
    char **input_array = split_input(input);
    struct File file_detail = check_file(input_array);
    pid_t new_p = fork();
    if (new_p < 0) {
        perror("fork failed");
        return;
    }
    if (new_p == 0) {
        signal(SIGINT, SIG_DFL);  
        if (file_detail.file_name != NULL) {
            int descriptor;
            if (file_detail.input_mode == 1) {
                descriptor = open(file_detail.file_name, O_RDONLY);
                if (descriptor < 0) {
                    perror("Failed to open input file");
                    exit(1);
                }
                dup2(descriptor, STDIN_FILENO);
                close(descriptor);
            } else {
                if (file_detail.append_mode == 1) {
                    descriptor = open(file_detail.file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    descriptor = open(file_detail.file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                if (descriptor < 0) {
                    perror("Failed to open output file");
                    exit(1);
                }
                dup2(descriptor, STDOUT_FILENO);
                close(descriptor);
            }
        }
        int execution = execvp(input_array[0], input_array);
        if (execution == -1) {
            printf("Input: %s\n", input_array[0]);
            printf("Your given input is not a system command\n");
            exit(1);
        }
    } else {
        wait(NULL);
    }
}


char *space_handling(char *str){
    int len = strlen(str);

    while (*str == ' '){
            str++;
        }
    char *end_of_str = str + len-1;
    while(end_of_str>str && *end_of_str == ' '){
            *end_of_str = '\0';
            end_of_str--;
        
        }
    return str;
    }


char ** semicolon_split(char *input_str){
    
    static char *executable_com[100];
    char *com;
    int c = 0;
    char temp[1000];
//    strcpy(input_str, temp);


    for (int i = 0; input_str[i]!='\0'; i++){
        if (input_str[i] == '&' && input_str[i+1] == '&'){
            input_str[i] =';';
        for (int j = i + 1; input_str[j] != '\0'; j++) {
                input_str[j] = input_str[j + 1];
            }
        }

    }
    
    com = strtok(input_str, ";");
    
    while(com != NULL){
    char *final_command = space_handling(com);
    executable_com[c] = final_command;
    c++;
    com = strtok(NULL, ";");
        
    }
    
    executable_com[c] = NULL; 
    return executable_com;
    
}
void execute_piped_commands(char *input) {
    
    char *commands[100];
    int num_cmds = 0;

    char *cmd = strtok(input, "|");
    while (cmd != NULL) {
        commands[num_cmds++] = space_handling(cmd); 
        cmd = strtok(NULL, "|");
    }

    int pipe_fd[2];
    int in_fd = 0; 

    for (int i = 0; i < num_cmds; i++) {
        pipe(pipe_fd);

        pid_t pid = fork();

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);  
            
            dup2(in_fd, STDIN_FILENO);
            if (i != num_cmds - 1) {
                
                dup2(pipe_fd[1], STDOUT_FILENO);
            }
            close(pipe_fd[0]); 
            close(pipe_fd[1]);

            
            char **args = split_input(commands[i]);
            struct File file_detail = check_file(args);

            if (file_detail.file_name != NULL) {
                int descriptor;
                if (file_detail.input_mode == 1) {
                    descriptor = open(file_detail.file_name, O_RDONLY);
                    if (descriptor < 0) {
                        perror("Failed to open input file");
                        exit(1);
                    }
                    dup2(descriptor, STDIN_FILENO);
                    close(descriptor);
                } else {
                    if (file_detail.append_mode == 1) {
                        descriptor = open(file_detail.file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    } else {
                        descriptor = open(file_detail.file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }
                    if (descriptor < 0) {
                        perror("Failed to open output file");
                        exit(1);
                    }
                    dup2(descriptor, STDOUT_FILENO);
                    close(descriptor);
                }
            }

            execvp(args[0], args);
            perror("exec failed");
            exit(1);
        } else if (pid < 0) {
            perror("Fork failed");
            return;
        }

        wait(NULL); 
        close(pipe_fd[1]); 
        in_fd = pipe_fd[0]; 
    }
}

#define max_count 1000
int count = 0;
char *history[max_count];

void show_history(char *com, bool show) {

    if (show==true) {
        printf("\n[COMMAND HISTORY]\n");
        for (int i = 0; i < count; i++) {
            printf("%d: %s\n", i + 1, history[i]);
        }
    } else {
        if (count < max_count && com && strlen(com) > 0) {
            history[count] = strdup(com);
            count++;
        }
    }
}



int main() {
    signal(SIGINT, SIG_IGN);
    while (true) {
        show_terminal();
        char *input = input_read();
        if (input == NULL) {
            printf("\n");
            break;
        }

        if (strcmp(input,"history") != 0){
            show_history(input,false);
         
        }
        
        if (strcmp(input, "history") == 0) {
            show_history(NULL, true);
            continue;
        }   

        
    
        char **commands = semicolon_split(input);
        int cmd_count = 0;
        while (commands[cmd_count] != NULL) {
            if (strchr(commands[cmd_count], '|') != NULL) {
                execute_piped_commands(commands[cmd_count]);
            } else {
                execute_system_command(commands[cmd_count]);
            }
            cmd_count++;
        }

                
    }
    return 0;
}

