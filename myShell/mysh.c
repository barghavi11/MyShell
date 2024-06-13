#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <glob.h>
#include "arraylist.h"

#define BYTES 1024
#define DEBUG 0
#define TEST 0
#ifndef BUFLENGTH
#define BUFLENGTH 1024
#endif

int status;
int status2;
int pipeFound;
int mode;


void enterMode(int fd);
void executeCommand(arraylist_t *words, char *stdInput, char *stdOutput, int inputUsed, int outputUsed);
char *removeWhiteSpaces(char *string);
int readLine(char *command, int fd);
void parse_line(char *line);
char *getPath(char *program);
void wildcardExpansion(char *token, arraylist_t *words);
void execute_pipe_command(arraylist_t words, arraylist_t rightHalf, char *stdInput, char *stdOutput, char *stdInput2, char *stdOutput2);

int main(int argc, char **argv){
    // enter function, figure out what mode to open in 
    // 0 = interactive mode, 1 = batch mode
    if( argc == 2 ){
        int fd = open(argv[1], O_RDONLY);
        if (isatty(fd)){
            mode = 0;
            enterMode(STDIN_FILENO);
        }
        else{
            mode = 1;
            enterMode(fd);
        }
    }
    else if (argc == 1 ){
        if (isatty(STDIN_FILENO)){
            mode = 0;
            enterMode(STDIN_FILENO);
        }
        else {
            mode = 1;
            enterMode(STDIN_FILENO);
        }
    }
    else{
        printf("Error, invalid command and/or inputs\n");
    }

}

void enterMode(int fd){
    // 0 - interactive, 1 - batch
    char command[BYTES];
    if (mode == 0){
        printf("Welcome to my shell!\n");
        printf("Type 'exit' to exit the program.\n");
        
    }
    while(1){
        if (mode == 0){
            printf("mysh> ");
            fflush(stdout);
        }
        int bytes = readLine(command, fd);
        //printf("bytes: %d\n", bytes);

        
        char *line = removeWhiteSpaces(command);
        
        if(*(line) == '\0') {
            if (mode == 0) printf("nothing to read, try again\n");
            free(line);
            continue;
        }
        parse_line(line);
        free(line);
        if ((mode == 1) && (bytes == 0)) break;
    }
}


char *removeWhiteSpaces(char *string){
    int countBeginning = 0;
    int countEnd = 0;
    
    for (int i = 0; i < strlen(string); i++){
        if(*(string + i) == ' ') countBeginning++;
        else break;
    }
    if(DEBUG) printf("count beginning is %d\n", countBeginning); //COMMENT

    for (int i = strlen(string) - 1; i >= countBeginning; i--){
        if(*(string + i) == ' ') countEnd++;
        else break;
    }
    if(DEBUG) printf("count end is %d\n", countEnd); //COMMENT 

    char *newString = malloc(strlen(string) + 1 - countEnd - countBeginning);
    for (int i = 0; i <strlen(string) - countEnd - countBeginning; i++){
        *(newString + i) = *(string + i + countBeginning);
    }
    newString[strlen(string) - countEnd - countBeginning] = '\0';
    if(DEBUG) printf("line without whitespace is '%s'\n", newString);

    return newString;
}

void wildcardExpansion(char *token, arraylist_t *words){
    // expands wildcard phrases and adds to arraylist
    glob_t pglob;
    int x;
    
    x = glob(token, GLOB_NOCHECK, NULL, &pglob);
    if (x == -1) perror("glob");     
      
    for (int i = 0; i < pglob.gl_pathc; i++){
        char *word = malloc(sizeof(char) *(strlen(pglob.gl_pathv[i]) + 1));
        strcpy(word, pglob.gl_pathv[i]);
        word[strlen(pglob.gl_pathv[i])] = '\0';
        al_push(words, word);
        
    
    }

    globfree(&pglob);


}

void parse_line(char *line){
    arraylist_t words;
    al_init(&words, 10);
    arraylist_t rightHalf;
    al_init(&rightHalf, 10);

    char *stdInput = malloc(sizeof(char) * (strlen(line) + 1));
    char *stdOutput = malloc(sizeof(char) * (strlen(line) + 1));
    char *stdInput2 = malloc(sizeof(char) * (strlen(line) + 1));
    char *stdOutput2 = malloc(sizeof(char) * (strlen(line) + 1));  
    char *rightHalfOfPipeWord = malloc(sizeof(char) * (strlen(line) + 1));
   
    // holds standard input for left and right side of pipe words , if pipe exists

   
    int inputUsed = 0;
    int outputUsed = 0;
    int inputUsed2 = 0;
    int outputUsed2 = 0;
    int redirection = 0;
    

    int breakLoop = 0;
    char *token = strtok(line, " ");

    
    // handles conditionals "then" and "else", depending on if pipe exists or not
    if (strcmp(token, "then") == 0){
        if(pipeFound){
        if(DEBUG) printf("the conditional token is %s\n", token);
            if (status != EXIT_SUCCESS || status2 != EXIT_SUCCESS) {
                free(stdInput);
                free(stdOutput);
                free(stdInput2);
                free(stdOutput2);
                free(rightHalfOfPipeWord);
                al_destroy(&words);
                al_destroy(&rightHalf);
                return;
            }
            else token = strtok(NULL, " ");
        }
        else{
            if (status != EXIT_SUCCESS){
                free(stdInput);
                free(stdOutput);
                free(stdInput2);
                free(stdOutput2);
                free(rightHalfOfPipeWord);
                al_destroy(&words);
                al_destroy(&rightHalf);
                return;
            }
            else token = strtok(NULL, " ");
        }
    }
    else if (strcmp(token, "else") == 0){
        if (pipeFound){
            if (status == EXIT_SUCCESS || status2 == EXIT_SUCCESS) {
                free(stdInput);
                free(stdOutput);
                free(stdInput2);
                free(stdOutput2);
                free(rightHalfOfPipeWord);
                al_destroy(&words);
                al_destroy(&rightHalf);
                return;
            }
            else token = strtok(NULL, " ");
        }
        else{
            if (status == EXIT_SUCCESS) {
                free(stdInput);
                free(stdOutput);
                free(stdInput2);
                free(stdOutput2);
                free(rightHalfOfPipeWord);
                al_destroy(&words);
                al_destroy(&rightHalf);
                return;
            }
            else token = strtok(NULL, " ");
        }
    }
    
    pipeFound = 0;

    // this entire loop takes care of cases such as if there were no space-seperated tokens 
    // between words or special characters (<, >, or |)
    while(token != NULL){
        if(DEBUG) printf("new token is %s\n", token);
        if(DEBUG) printf("redirection is %d\n", redirection);
        int prevToken = 0;
        // 0 = no redirection, 1 = "<", 2 = ">"

        if (strchr(token, '>') || strchr(token, '<') || strchr(token, '|')){

            if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, "|") == 0){
                if(DEBUG) printf("token is %s\n", token);
                if (strcmp(token, "<") == 0){
                    redirection = 1;
                }
                if (strcmp(token, ">") == 0){
                    redirection = 2;

                }
                if(strcmp(token, "|") == 0){
                    free(rightHalfOfPipeWord);
                    pipeFound = 1;
                    break;
                

                }


                }   
            else{

                for (int i = 0; i<strlen(token); i++){
                    // if the token itself is < or > 
                    
                    if(*(token + i) == '<' || *(token + i) == '>'){
                        //push the word if there was no redirection token before
                        if (redirection == 0){
                            if ((i - prevToken )!=0){
                                char *word = malloc((i - prevToken + 1) * sizeof(char));
                                strncpy(word, token + prevToken, i-prevToken); 
                                word[i - prevToken] = '\0';
                                if(DEBUG) printf("word: %s\n", word);
                                al_push(&words, word); 

                            }
                        }
                        else if(redirection == 1){ 
                            // make the token standard input
                            strncpy(stdInput, token + prevToken, i-prevToken);
                            stdInput[i-prevToken] = '\0';
                            if(DEBUG) printf("stdinput: %s\n", stdInput);
                            redirection = 0;
                            inputUsed = 1;

                        }
                        else{
                            strncpy(stdOutput, token + prevToken, i-prevToken); 
                            stdOutput[i-prevToken] = '\0';
                            if(DEBUG) printf("stdoutput: %s\n", stdOutput);
                            outputUsed = 1;

                            redirection = 0;
                        }

                        if(*(token + i) == '<'){
                            // if its a redirection
                            redirection = 1;
                        }
                        else if (*(token + i) == '>'){
                            redirection = 2;
                        }

                        prevToken = i + 1;

                    }
                    else if (*(token + i) == '|'){
                        if(redirection ==1 ){
                            strncpy(stdInput, token + prevToken, i-prevToken);
                            stdInput[i-prevToken] = '\0';
                            redirection = 0;
                            inputUsed = 1;

                        }
                        else if(redirection == 2){
                            strncpy(stdOutput, token + prevToken, i-prevToken); 
                            stdOutput[i-prevToken] = '\0';
                            outputUsed = 1;
                            redirection = 0;

                        }
                        else{
                            if((i - prevToken) != 0){
                            char *word = malloc(i - prevToken + 1);
                            strncpy(word, token + prevToken, i - prevToken);
                            word[i - prevToken] = '\0';
                            al_push(&words, word);
                                }
                        }

                        if((strlen(token) - 1 - i) != 0){
                            strcpy(rightHalfOfPipeWord, token + i + 1);

                            rightHalfOfPipeWord[strlen(token) - i - 1] = '\0'; 
                            al_push(&rightHalf, rightHalfOfPipeWord);
                        }
                        else{
                            free(rightHalfOfPipeWord);
                        }
                        
                        pipeFound = 1;
                        breakLoop = 1;
                        break;
                    }

            }
            // if pipe is found, break loop;
            if (breakLoop) break;
            if((strlen(token) - prevToken ) !=0 ){
                if (redirection == 1){
                    strcpy(stdInput, token + prevToken);
                    redirection = 0;
                    inputUsed = 1;
                }
                else if(redirection == 2){
                    strcpy(stdOutput, token + prevToken);
                    redirection = 0;
                    outputUsed = 1;
                }
                else{
                    char *word = malloc((strlen(token) - prevToken + 1) * sizeof(char));
                    strncpy(word, token + prevToken, strlen(token)-prevToken); 
                    word[strlen(token)-prevToken] = '\0';
                    al_push(&words, word); 

                }
            }
            }

        }
        else if(redirection != 0){
            
            if (redirection == 1) {
                
                strcpy(stdInput, token); 
                redirection = 0;
                inputUsed = 1;
            }
            if (redirection == 2) {
                strcpy(stdOutput, token); 
                redirection = 0;
                outputUsed = 1;
            }

        }
        else if (strchr(token, '*')){
            // malloc in here
            wildcardExpansion(token, &words);
            for (int i = 0; i<words.length; i++){
            }
             
        }
        else{
            char *word = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(word, token);
            word[strlen(token)] = '\0';
            al_push(&words, word);
            if(DEBUG) printf("word :%s\n", words.data[words.length - 1]);
        }

        token = strtok(NULL, " ");

    }

    if(pipeFound){

        int redirection = 0;
        // 0 = no redirection, 1 = "<", 2 = ">"
        
        token = strtok(NULL, " ");
        while(token != NULL){
        int prevToken = 0;
        // 0 = no redirection, 1 = "<", 2 = ">"
        if (strchr(token, '>') || strchr(token, '<') || strchr(token, '|')){
            if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0 ){
                if (strcmp(token, "<") == 0){
                    redirection = 1;
                }
                if (strcmp(token, ">") == 0){
                    redirection = 2;

                }
            }   
            else{

                for (int i = 0; i<strlen(token); i++){
                    // if the token itself is < or > 
                    
                    if(*(token + i) == '<' || *(token + i) == '>'){
                        //push the word if there was no redirection token before
                        if (redirection == 0){
                            if ((i - prevToken )!=0){
                                char *word = malloc((i - prevToken + 1) * sizeof(char));
                                strncpy(word, token + prevToken, i-prevToken); 
                                word[i - prevToken] = '\0';
                                if(DEBUG) printf("word: %s\n", word);
                                al_push(&rightHalf, word); 

                            }
                        }
                        else if(redirection == 1){ 
                            // make the token standard input
                            strncpy(stdInput2, token + prevToken, i-prevToken);
                            stdInput2[i-prevToken] = '\0';
                            if(DEBUG) printf("stdinput: %s\n", stdInput2);
                            redirection = 0;
                            inputUsed2 = 1;

                        }
                        else{
                            strncpy(stdOutput2, token + prevToken, i-prevToken); 
                            stdOutput2[i-prevToken] = '\0';
                            if(DEBUG) printf("stdoutput: %s\n", stdOutput2);
                            outputUsed2 = 1;

                            redirection = 0;
                        }

                        if(*(token + i) == '<'){
                            // if its a redirection
                            redirection = 1;
                        }
                        else if (*(token + i) == '>'){
                            redirection = 2;
                        }

                        prevToken = i + 1;

                    }

            }
            if((strlen(token) - prevToken ) !=0 ){
                if (redirection == 1){
                    strcpy(stdInput2, token + prevToken);
                    redirection = 0;
                    inputUsed2 = 1;
                }
                else if(redirection == 2){
                    strcpy(stdOutput2, token + prevToken);
                    redirection = 0;
                    outputUsed2 = 1;
                }
                else{
                    char *word = malloc((strlen(token) - prevToken + 1) * sizeof(char));
                    strncpy(word, token + prevToken, strlen(token)-prevToken); 
                    word[strlen(token)-prevToken] = '\0';
                    al_push(&rightHalf, word); 

                }
            }
            }

        }
        else if(redirection != 0){
            
            if (redirection == 1) {
                
                strcpy(stdInput2, token); 
                redirection = 0;
                inputUsed2 = 1;
            }
            if (redirection == 2) {
                strcpy(stdOutput2, token); 
                redirection = 0;
                outputUsed2 = 1;
            }

        }
        else if (strchr(token, '*')){
            // expand wildcard if exists
            wildcardExpansion(token, &words);
             
        }
        else{
            char *word = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(word, token);
            word[strlen(token)] = '\0';
            al_push(&rightHalf, word);
            if(DEBUG) printf("word :%s\n", rightHalf.data[rightHalf.length - 1]);
        }

        token = strtok(NULL, " ");

    }
        
        if (!inputUsed2){
            free(stdInput2);
            stdInput2 = NULL;
        }
        if(!outputUsed2){
            free(stdOutput2);
            stdOutput2 = NULL;
        }
        if (!inputUsed){
            free(stdInput);
            stdInput = NULL;
        }
        if (!outputUsed){
            free(stdOutput);
            stdOutput = NULL;
        }
        
        execute_pipe_command(words, rightHalf, stdInput, stdOutput, stdInput2, stdOutput2);
       
        al_destroy(&words);
        al_destroy(&rightHalf);
        if(inputUsed) free(stdInput);
        if(outputUsed) free(stdOutput);
        if(inputUsed2) free(stdInput2);
        if(outputUsed2) free(stdOutput2);
        return;

    }
    else{
        free(rightHalfOfPipeWord);
    }
    
    executeCommand(&words, stdInput, stdOutput, inputUsed, outputUsed);
    al_destroy(&words);
    al_destroy(&rightHalf);
    free(stdInput);
    free(stdOutput);
    free(stdInput2);
    free(stdOutput2);

}

void execute_pipe_command(arraylist_t words, arraylist_t rightHalf, char *stdInput, char *stdOutput, char *stdInput2, char *stdOutput2){
    int pipe_fdes[2];
    // if command contains pipe, pipe() and then use corresponding arraylists to execute the commands
    pid_t pid, pid2;
    int input = 0;
    int output = 0;
    int input2 = 0;
    int output2 = 0;

    if (pipe(pipe_fdes) == -1) {
        perror("error using pipe\n");
        exit(EXIT_FAILURE);
    }
    if (stdInput != NULL){
        input = 1;
            }
    if(stdOutput !=NULL){
        output = 1;
            }
    if (stdInput2 != NULL){
        input2 = 1;              
        }

    if(stdOutput2 !=NULL){
        output2 = 1;    
        }
    // if the command is cd, handle seperately.
    if (strcmp(words.data[0], "cd") == 0){
        int fd_in;
        int fd_out;
        if(input){
            fd_in = open(stdInput, O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
        }
        if (output){
            
            fd_out = open(stdOutput, O_CREAT | O_WRONLY | O_TRUNC, 0640);
            dup2(fd_out, STDOUT_FILENO);

        }
        int x = chdir(words.data[1]);
        if (x == -1) {
            printf("error changing directory\n");
            status = EXIT_FAILURE;
        }
        if (input) close(fd_in);
        if (output) close(fd_out);

    }
    // cd command on right side of pipe
    if (strcmp(rightHalf.data[0], "cd") == 0){
        int fd_in;
        int fd_out;
        if(input){
            fd_in = open(stdInput2, O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
            }
        if (output){
            
            fd_out = open(stdOutput2, O_CREAT | O_WRONLY | O_TRUNC, 0640);
            dup2(fd_out, STDOUT_FILENO);

        }
        int x = chdir(rightHalf.data[1]);
        if (x == -1) {
            printf("error changing directory\n");
            status2 = EXIT_FAILURE;
        }
        if (input) close(fd_in);
        if (output) close(fd_out);

    }
        
        pid = fork();
        if( pid == 0){
            // child process

            close(pipe_fdes[0]);
            dup2(pipe_fdes[1], STDOUT_FILENO);
            close(pipe_fdes[1]);

            executeCommand(&words, stdInput, stdOutput, input, output);
            exit(EXIT_FAILURE);

        }
        
        if((strcmp(words.data[0], "exit") == 0)){
            wait(NULL);
            exit(EXIT_SUCCESS);
        }
        pid2 = fork();
        if(pid2 == 0){
            // child 2 process
            close(pipe_fdes[1]);
            dup2(pipe_fdes[0], STDIN_FILENO);
            close(pipe_fdes[0]);
            
            executeCommand(&rightHalf,stdInput2, stdOutput2, input2, output2);
            exit(EXIT_SUCCESS);


        }

        if((strcmp(rightHalf.data[0], "exit") == 0)){
            exit(EXIT_SUCCESS);
        }

        // parent 
        close(pipe_fdes[0]);
        close(pipe_fdes[1]);
        wait(&status);
        wait(&status2);
        return;

}


void executeCommand(arraylist_t *words, char *stdInput, char *stdOutput, int inputUsed, int outputUsed){
    // execute a given command ignoring / without pipes
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);
    
    if (strcmp(words->data[0], "pwd") == 0){
        if(inputUsed){
            int fd_in = open(stdInput, O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
            }
        if (outputUsed){
            int fd_out = open(stdOutput, O_CREAT | O_WRONLY | O_TRUNC, 0640);
            dup2(fd_out, STDOUT_FILENO);

        }
        char buf[BUFLENGTH];
        printf("%s\n", getcwd(buf, BUFLENGTH));
        if (getcwd(buf, BUFLENGTH) == NULL){
            printf("error getting current working directory\n");
            status = EXIT_FAILURE;
        }

        if (inputUsed) dup2(saved_stdin, STDIN_FILENO);
        if (outputUsed) dup2(saved_stdout, STDOUT_FILENO);
    }
    else if (strcmp(words->data[0], "cd") == 0){
        if(inputUsed){
            int fd_in = open(stdInput, O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
            }
        if (outputUsed){
            
            int fd_out = open(stdOutput, O_CREAT | O_WRONLY | O_TRUNC, 0640);
            dup2(fd_out, STDOUT_FILENO);

        }
        int x = chdir(words->data[1]);
        if (x == -1) {
            printf("error changing directory\n");
            status = EXIT_FAILURE;
        }
        if (inputUsed) dup2(saved_stdin, STDIN_FILENO);
        if (outputUsed) dup2(saved_stdout, STDOUT_FILENO);

    }
    else if (strcmp(words->data[0], "which") == 0){
        if(inputUsed){
            int fd_in = open(stdInput, O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
            }
        if (outputUsed){
        
            int fd_out = open(stdOutput, O_CREAT | O_WRONLY | O_TRUNC, 0640);
            dup2(fd_out, STDOUT_FILENO);
        }
        if (words->length != 2){
            printf("error, expected an argument\n");
            status = EXIT_FAILURE;
            return;
        }
        char *path = getPath(words->data[1]);
        if (path == NULL){
            status = EXIT_FAILURE;
        }
        else {
            printf("%s\n", path);
            free(path);
        }
        
        if (inputUsed) dup2(saved_stdin, STDIN_FILENO);
        if (outputUsed) dup2(saved_stdout, STDOUT_FILENO);
        

    }
    else if (strcmp(words->data[0], "exit") == 0){
        dup2(0, STDOUT_FILENO);
        for (int i = 1; i< words->length; i++){
            printf("%s ", words->data[i]);
        }
        if(words->length > 1){
            printf("\n");
        }
        if (mode == 0) printf("mysh: exiting\n");
        exit(EXIT_SUCCESS);
    }
    else {
        pid_t pid = fork();
        if (pid == -1) {
            perror("error using fork\n");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0 ){
            
            al_push(words, NULL);
            if(strchr(words->data[0], '/')){
               
                execv(words->data[0], words->data);
                exit(EXIT_FAILURE);
            }
            char *path = getPath(words->data[0]);
            
            if(inputUsed){
                int fd_in = open(stdInput, O_RDONLY);
                dup2(fd_in, STDIN_FILENO);
            }
            if (outputUsed){
                
                int fd_out = open(stdOutput, O_CREAT | O_WRONLY | O_TRUNC, 0640);
                dup2(fd_out, STDOUT_FILENO);

            }
            
            execv(path, words->data);
            exit(EXIT_FAILURE);
        }
        else{

            wait(&status);
        }

        
    }

}

char *getPath(char *program){
    // get path for a command to execute
    char *path = malloc(strlen("/usr/local/bin/") + strlen(program) + 1);
    strcpy(path, "/usr/local/bin/");
    strcat(path, program);
    if(access(path, R_OK)==0){
        return path;
    }
    else{
        strcpy(path, "/usr/bin/");
        strcat(path, program);

        if(access(path, R_OK)==0){
            return path;
        }
        else{
            strcpy(path, "/bin/");
            strcat(path, program);
            if(access(path, R_OK)==0){
                return path;
            }
            else {
                free(path);
                return NULL;
            }
        }
    }
}

int readLine(char *command, int fd){
    int bytes;
    char *buf = malloc(1);
    int index = 0;

    while((bytes = read(fd, buf, 1)) == 1 ){
        if (*buf != '\n') {
            command[index] = *buf;
            index ++;
        }
        else{
            command[index] = '\0';
            index++;
            free(buf);
            return bytes;
        }
    }
    if (index !=0){
        command[index] = '\0';
    }
    free(buf);
    return bytes;
   
}