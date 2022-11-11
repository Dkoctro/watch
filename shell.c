#ifndef _POSIX_C_SOURCE
	#define  _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>


void welcome_message(); // Print the welcome message
void loop();
char* read_command();
char** split_command(char *input);
int launch(char **args);
int execute_command(char** args);
int help(char** args);
int cd(char** args);
int echo(char** args);
int exit_shell(char** args);
int record(char** args);
int replay(char** args);
int mypid(char** args);
void store_command(char* input);
void redirection_in(char** args);
void redirection_out(char** args);
int pipeline(char** args);

char **history; // Two dimensional array to record past commands
int position = 0; // To record the number of past commands
int redirectIn = 0;
int redirectOut = 0;
int background = 0;
int pi = 0;
int pi_read = 0;
int replay_cmd = 0;


int main(){
	welcome_message();
	loop();
	return 0;
}

void loop(){
	char *input;
	char **split;
	int status;
	do{
		printf(">>> $ ");
		input = read_command();
		if(input[0] ==  '\n' || input[0] == '\t' || input[0] == ' '){
			continue;
		}
		for(int i = 0; i < strlen(input); i++){
			if(input[i] == '<')
				redirectIn = 1;
		 	else if(input[i] == '>')
				redirectOut = 1;
			else if(input[i] == '&')
				background = 1;
			else if(input[i] == '|')
				pi++;
		}
		if(background){
			input[strlen(input)-1] = '\0';
		}
		split = split_command(input);
		
		if(pi){
			status = pipeline(split);
		} else {
			status = execute_command(split);
		}
		redirectIn = 0;
		redirectOut = 0;
		background = 0;
		pi = 0;
		pi_read = 0;
		replay_cmd = 0;
		free(input);
		free(split);
	}while(status);
	
}

char* read_command(){
	char *input;
	size_t size = 16;
	size_t input_size = 0;
	input = (char*)malloc(sizeof(char) * size);
	input_size = getline(&input, &size, stdin);
	if(input[0] ==  '\n' || input[0] == '\t' || input[0] == ' '){
		return "\n";
	}
		
	char* store;
	store = (char*)malloc(sizeof(char) * (input_size -1));
	// for(int i = 0; i < input_size-1; i++){
	// 	store[i] = input[i];
	// }
	strcpy(store, input);
	
	store_command(store);
	
	free(input);
	return store;
}

char** split_command(char *input){
	int buffersize = 64, pos = 0;
	char** split = malloc(buffersize * sizeof(char*));
	char *token;
	char delim[] = " \t\r\n\a";
	
	token = strtok(input, delim);
	while(token != NULL){
		split[pos] = token;
		pos++;

		if(pos >= buffersize){
			buffersize += 64; // Add the number of string which can be put into split string array
			split = realloc(split, buffersize * sizeof(char*));
		}

		token = strtok(NULL, delim);
	}
	split[pos] = NULL;
	if(!strcmp(split[0], "replay")){
		position--;
		history[position] = NULL;
	}
	if(background && !strcmp(split[0], "replay")){
		replay(split);
	}
	return split;
}

int launch(char **args){
	pid_t pid, wpid;
	int status;

	pid = fork();
	if(pid < 0){
		perror("lError");
	} else if(pid == 0){
		if(!pi){
			redirection_in(args);
			redirection_out(args);
		}
		// args[1] = (char*)malloc(sizeof(char) * 10);
		// strcpy(args[1], "text.txt");
		// for(int j = 0; args[j] != NULL; j++){
		// 	perror(args[j]);
		// // }
		if(execvp(args[0], args) == -1){
			perror("Error");
		}
		perror(args[0]);
		exit(1);
	} else {
		do{
			wpid = waitpid(pid, &status, WUNTRACED);
		}while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int execute_command(char** args){
	if(!strcmp(args[0], "\n") || !strcmp(args[0], "\t") || !strcmp(args[0], " ")){
		return 1;
	}
	if(background && replay_cmd == 0){
		int status;
		int built_in = 0;
		pid_t pid;
		pid = fork();
		if(pid == 0){
			int tmp = pi + 1;
			if(pi_read ==  tmp || pi == 0)
				printf("[Pid]: %d\n", getpid());

			if(!strcmp(args[0], "help")){
				help(args);
				built_in = 1;
			}
			if(!strcmp(args[0], "cd")){
				cd(args);
				built_in = 1;
			}
			if(!strcmp(args[0], "echo")){
				echo(args);
				built_in = 1;
			}
			if(!strcmp(args[0], "exit")){
				exit_shell(args);
				built_in = 1;
			}
			if(!strcmp(args[0], "record")){
				record(args);
				built_in = 1;
			}
			if(!strcmp(args[0], "replay")){
				replay_cmd = 1;
				replay(args);
				built_in = 1;
			}
			if(!strcmp(args[0], "mypid")){
				mypid(args);
				built_in = 1;
			}  
			if(!built_in)
				launch(args);
			exit(1);
		} else if(pid > 0){
			waitpid(pid, &status, WUNTRACED);
			return 1;
		}
	} else {
		if(!strcmp(args[0], "help")){
			return help(args);
		}
		if(!strcmp(args[0], "cd")){
			return cd(args);
		}
		if(!strcmp(args[0], "echo")){
			return echo(args);
		}
		if(!strcmp(args[0], "exit")){
			return exit_shell(args);
		}
		if(!strcmp(args[0], "record")){
			return record(args);
		}
		if(!strcmp(args[0], "replay")){
			replay_cmd = 1;
			return replay(args);
		}
		if(!strcmp(args[0], "mypid")){
			return mypid(args);
		}
	}

	return launch(args);
}

// Built-in command 

int help(char** args){ // Finish but not test
	int backup = dup(STDOUT_FILENO);
	redirection_out(args);
	printf("----------------------------------------------------------------\n"
		   "my little shell                                                 \n"
		   "Type program names and arguments, and hit enter.                \n"
		   "                                                                \n"
		   "The following are built in:                                     \n"
		   "1: help:   show all build-in function info                      \n"
		   "2: cd:     change directory                                     \n"
		   "3: echo:   echo the strings to standard output                  \n"
		   "4: record: show last 16 cmds you typed in                       \n"
		   "5: replay: re-execute the cmd showed in record                  \n"
		   "6: mypid:  find and print process-ids                           \n"
		   "7: exit:   exit shell                                           \n"
		   "                                                                \n"
		   "Use the \"man\" command for information on other programs         \n"
		   "----------------------------------------------------------------\n");
	dup2(backup, STDOUT_FILENO);
	return 1;
}

int cd(char** args){ // Finish but not test
	if(chdir(args[1]) != 0){
		perror("Error");
	}
	return 1;
}

int echo(char** args){ // Finish but not test
	int backup = dup(STDOUT_FILENO);
	redirection_out(args);
	if(!strcmp(args[1], "-n")){ // Output doesn't need newline
		int i = 2;
		while(1){
			printf("%s", args[i]);
			i++;
			if(args[i] != NULL){
				printf(" ");
			} else {
				break;
			}
		}
	} else {
		int i = 1;
		while(1){
			printf("%s", args[i]);
			i++;
			if(args[i] != NULL){
				printf(" ");
			} else {
				break;
			}
		}
		printf("\n");
	}
	dup2(backup, STDOUT_FILENO);
	return 1;
}

int exit_shell(char** args){ // Finish but not test
	printf("my little shell: See you next time!\n");
	return 0;
}

int record(char** args){ // Finish but not test
	int backup = dup(STDOUT_FILENO);
	redirection_out(args);
	printf("history cmd:\n");
	for(int i = 0; i < 16; i++){
		if(i < position)	
			printf("%d: %s\n", i+1, history[i]);
		// else
		// 	printf("%d:   \n", i+1);
	}
	dup2(backup, STDOUT_FILENO);
	return 1;
}

int replay(char** args){ // Finish but not test
	char* input;
	int number = atoi(args[1]);
	if(number >= 1 && number <= position){
		if(redirectOut == 0){
			input = (char*)malloc(sizeof(char) * strlen(history[number-1]));
			strcpy(input, history[number-1]);
			char* tmp = (char*)malloc(sizeof(char) * strlen(input));
			strcpy(tmp, input);
			if(background){
				strcat(tmp, " ");
				strcat(tmp, "&");
				store_command(tmp);
			} else
				store_command(input);
			char **split = split_command(input);
			free(input);
			if(background == 0 || (background == 1 && replay_cmd == 1))
				execute_command(split);
		} else {
			input = (char*)malloc(sizeof(char) * strlen(history[number-1]));
			strcpy(input, history[number-1]);
			int i;
			for(i = 0; args[i] != NULL; i++){
				if(!strcmp(args[i], ">"))
					break;
			}
			strcat(input, " ");
			strcat(input, args[i]);
			strcat(input, " ");
			strcat(input, args[i+1]);
			char* tmp = (char*)malloc(sizeof(char) * strlen(input));
			strcpy(tmp, input);
			if(background){
				strcat(tmp, " ");
				strcat(tmp, "&");
				store_command(tmp);
			} else
				store_command(input);
			char **split = split_command(input);
			free(input);
			if(background == 0)
				execute_command(split);
		}
	} else {
		char error_message[] = "replay: wrong args";
		printf("%s\n", error_message);
	}
	return 1;
}

int mypid(char** args){
	char myself[] = "-i"; // Find mypid
	char parent[] = "-p"; // Find the pid of the pid of parent (The number will be the children)
	char children[] = "-c"; // Find the pid of the pid of children (The number will be the parent);
	int backup = dup(STDOUT_FILENO);
	redirection_out(args);
	if(!strcmp(args[1], myself)){
		printf("%d\n", getpid());
	} else if(!strcmp(args[1], parent)){
		char* path = (char*)malloc(sizeof(char) * 1024);
		strcpy(path, "/proc/");
		strcat(path, args[2]);
		strcat(path, "/status");

		FILE *file = fopen(path, "r");
		if(file == NULL){
			printf("mypid -p: process id not exist\n");
		} else {
			char *input = (char*)malloc(sizeof(char)*1024);
			size_t size;
			size_t input_size;
			char *ppid;
			char *tmp;
			while(input_size = getline(&input, &size, file) != EOF){
				if(input[0] == 'P' && input[1] == 'P' && input[2] == 'i' && input[3] == 'd'){
					tmp = (char*)malloc(sizeof(char) * strlen(input));
					strcpy(tmp, input);
					int j;
					for(j = 0; tmp[j] != '\n'; j++){
						if(tmp[j] >= 48 && tmp[j] <= 57){
							break;
						}
					}
					ppid = (char*)malloc(sizeof(char) * 1024);
					for(int k = 0; tmp[j] != '\n'; j++, k++){
						ppid[k] = tmp[j];
					}
					break;
				}
			}
			printf("%s\n", ppid);
		}
		free(path);
	} else if(!strcmp(args[1], children)){
		char* path = (char*)malloc(sizeof(char) * 1024);
		strcpy(path, "/proc/");
		strcat(path, args[2]);
		strcat(path, "/task/");
		strcat(path, args[2]);
		strcat(path, "/children");

		FILE *file = fopen(path, "r");
		char *input = (char*)malloc(sizeof(char) * 1024);
		size_t size;
		size_t input_size;
		while(getline(&input, &size, file) != EOF){
			if(input_size == 0)
				break;
			printf("%s\n", input);
		}
		free(path);	
	}
	dup2(backup, STDOUT_FILENO);
	
	return 1;
}

void store_command(char* input){ // Use to store the command
	
	if(position == 0) // If this is the first command, malloc the memory space
		history = (char**)malloc(sizeof(char*) * 16); // record 16 past command

	if(position < 16){ // If the number of past command aren't more than 16, store the command on the bottom
		history[position] = (char*)malloc(sizeof(char) * strlen(input));
		
		// for(int i = 0; i < strlen(input); i++){
		// 	history[position][i] = input[i];
		// }
		strcpy(history[position], input);
		position++;
	} else { // position = 16, remove first command and append the command on the bottom
		char* remove_command;
		remove_command = (char*)malloc(sizeof(char) * strlen(history[0]));
		strcpy(remove_command, history[0]);
		for(int i = 0; i < position-1; i++){
			history[i] = (char*)malloc(sizeof(char) * strlen(history[i+1]));
			strcpy(history[i], history[i+1]);
		}
		history[position-1] = (char*)malloc(sizeof(char) * strlen(input));
		strcpy(history[position-1], input);
	}
}

void welcome_message(){ // Finish but not test
	printf("====================================================\n"
		   "* Welcome to my little shell:                      *\n"
		   "*                                                  *\n"
		   "* Type \"help\" to see builtin functions             *\n"
		   "*                                                  *\n"
		   "* If you want to do things below:                  *\n"
		   "* + redirection: \">\" or \"<\"                        *\n"
		   "* + pipe: \"|\"                                      *\n"
		   "* + background: \"&\"                                *\n"
		   "* Make sure they are seperated by \"(space)\".       *\n"
		   "*                                                  *\n"
		   "* Have fun!!                                       *\n"
		   "====================================================\n");
}

void redirection_in(char** args){
	if(redirectIn){
		int i;
		for(i = 0; args[i] != NULL; i++){
			if(!strcmp(args[i], "<"))
				break;
		}
		
		char* input_file = (char*)malloc(sizeof(char) * 1024);
		strcpy(input_file, args[2]);
		args[1] = args[2] = NULL;
		if(redirectOut && !pi){
			args[1] = (char*)malloc(sizeof(char) * strlen(args[3]));
			args[2] = (char*)malloc(sizeof(char) * strlen(args[4]));
			strcpy(args[1], args[3]);
			strcpy(args[2], args[4]);
			args[3] = args[4] = NULL;
		}
		int in = open(input_file, O_RDONLY);
		dup2(in, STDIN_FILENO);
		close(in);
	}
}

void redirection_out(char** args){
	if(redirectOut){
		int i;
		for(i = 0; args[i] != NULL; i++){
			if(!strcmp(args[i], ">"))
				break;
		}
		
		char* output_file = (char*)malloc(sizeof(char) * strlen(args[i+1]));
		
		strcpy(output_file, args[i+1]);
		args[i] = args[i+1] = NULL;
		int out = creat(output_file, 0644);
		dup2(out, STDOUT_FILENO);
		close(out);
	}
}

int pipeline(char** args){
	
	char **command[pi+1];
	int pos = 0;
	int j = 0;
	int i = 0;
	for(i = 0; args[i] != NULL; i++){
		if(!strcmp(args[i], "|")){
			char** tmp;
			tmp = (char**)malloc(sizeof(char*) * (i - j));
			for(int k = 0; k < i - j; k++){
				tmp[k] = (char*)malloc(sizeof(char) * strlen(args[k+j]));
				strcpy(tmp[k], args[k+j]);
			}
			command[pos] = (char**)malloc(sizeof(char*) * (i - j));
			for(int k = 0; k < i - j; k++){
				command[pos][k] = (char*)malloc(sizeof(char) * strlen(tmp[k]));
				strcpy(command[pos][k], tmp[k]);
			}
			pos++;
			j = i + 1;
		}
	}

	// Last part
	char** tmp;
	tmp = (char**)malloc(sizeof(char*) * (i - j));
	for(int k = 0; k < i - j; k++){
		tmp[k] = (char*)malloc(sizeof(char) * strlen(args[k+j]));
		strcpy(tmp[k], args[k+j]);
	}
	command[pos] = (char**)malloc(sizeof(char*) * (i - j));
	for(int k = 0; k < i - j; k++){
		command[pos][k] = (char*)malloc(sizeof(char) * strlen(tmp[k]));
		strcpy(command[pos][k], tmp[k]);
	}

	int pipefd[pi][2]; // 0 for read, 1 for write
	for(i = 0; i < pi; i++){
		pipe(pipefd[i]);
	}
	
	pid_t pid;
	for(i = 0; i < pi+1; i++){
		pi_read++;
		pid = fork();
		if(pid == 0){
			if(i == 0 && redirectIn){
				int j;
				for(j = 0; command[0][j] != NULL; j++){
					if(!strcmp(command[0][j], "<"))
						break;
				}
				command[0][j] = (char*)malloc(sizeof(char) * strlen(command[0][j+1]));
				command[0][j] = command[0][j+1];
				command[0][j+1] = NULL;
				char* input_file = command[0][j];
				command[0][j] = NULL;
				int in = open(input_file, O_RDONLY);
				if(dup2(in, STDIN_FILENO) < 0)
					perror("DUPDUPDUPDUP");
				close(in);
				redirectIn = 0;
			}
			else if(i != 0){
				dup2(pipefd[i-1][0], 0);
			}

			if(i == pi){
				redirection_out(command[pi]);
				redirectOut = 0;
			}
			else if(i != pi){
				dup2(pipefd[i][1], 1);
			}

			for(int j = 0; j < pi; j++){
				close(pipefd[j][0]);
				close(pipefd[j][1]);
			}
			
			execute_command(command[i]);
			exit(0);
		} else {
			if(i != 0){
				close(pipefd[i-1][0]);
				close(pipefd[i-1][1]);
			}
		}
	}

	waitpid(pid, NULL, 0);
	return 1;
}
