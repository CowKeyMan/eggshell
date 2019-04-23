#include "commandsManager.h"
#include "parser.h"
#include "processManager.h"

void printCommand(char** args){
    bool during_Text = 0; //bool to indicate if the text has ""

    for (int i = 1; args[i] != NULL; i++) { //start from 1 since args[0] is 'print'
        if (during_Text == false) {
            //if arg starts with " it means that text has started
            if (args[i][0] == '"' || args[i][0] == '$') {
                //get string without the first character
                char c[VARIABLE_LENGTH];
                strncpy(c, trimFirstLetter(args[i]), VARIABLE_LENGTH);
                if (args[i][0] == '"') {
                    during_Text = true;
                    for (int j = 1; args[i][j] != '\0'; j++) {
                        if (args[i][j] == '"' && args[i][j + 1] == '\0') {
                            during_Text = false;
                            c[j - 1] = '\0';
                            continue;
                        }
                        c[j - 1] = args[i][j];
                    }
                    if(args[i+1] != NULL) {
                        printf("%s ", c);
                    }else{
                        printf("%s", c);
                    }
                }
                //if arg start with $ print the variable. Note: if variable is not found, null is printed
                else if (args[i][0] == '$') {
                    if(args[i+1] != NULL) {
                        printf("%s ", getVariable(c));
                    }else{
                        printf("%s", getVariable(c));
                    }
                }
            } else {
                if(args[i+1] != NULL) {
                    printf("%s ", args[i]);
                }else{
                    printf("%s", args[i]);
                }
            }
        } else {
            //check if the last variable is ", in which case end text
            char c[VARIABLE_LENGTH];
            for (int j = 0; args[i][j] != '\0'; j++) {
                if (args[i][j] == '"' && args[i][j + 1] == '\0') {
                    during_Text = false;
                    c[j] = '\0';
                    continue;
                }
                c[j] = args[i][j];
            }
            if(args[i+1] != NULL) {
                printf("%s ", c);
            }else{
                printf("%s", c);
            }
        }
    }
    printf("\n");
}

void chdirCommand(char* newDir){
    //Change the directory
    if (chdir(newDir) != 0) {
        fprintf(stderr, "Invalid path\n");
    }else {
        char buffer [VARIABLE_LENGTH];
        getcwd(buffer, VARIABLE_LENGTH);
        setVariable("CWD", buffer);
    }
}

void variableModificationCommand(char* line){
    //Itemsbuf  now stores the 2 items(variable name, variable new value)
    char* itemsBuf[2];
    //printf("%s %s\n", line, itemsBuf[1]);
    parseStringBy(line, "=", itemsBuf);

    if(itemsBuf[1] == NULL){
        fprintf(stderr, "Error: no assignment\n");
        return;
    }

    char items[2][VARIABLE_LENGTH];
    strncpy(items[0], itemsBuf[0], VARIABLE_LENGTH);
    strncpy(items[1], itemsBuf[1], VARIABLE_LENGTH);

    strncpy(items[0], trimFirstLetter(items[0]), VARIABLE_LENGTH); //Remove $ from variable name
    removeSpaces(items[0]);
    removeSpaces(items[1]);

    if(items[1][0] == '$'){
        strncpy(items[1], trimFirstLetter(items[1]), VARIABLE_LENGTH);
        if(getVariable(items[1]) != NULL) {
            strncpy(items[1], getVariable(items[1]), VARIABLE_LENGTH);
        }else{
            printf("Variable Does not exist\n");
            return;
        }
    }

    setVariable(items[0], items[1]);
}

void forkChild(char** args, int* inputPipe, int* outputPipe, bool readFromPipe, bool writeToPipe){

    //Boolean to get triggered false if the process is to run in the background
    bool parentWait = true;

    //If & is a parameter, remove all parameters after it and set trigger to put process in background
    for(int i = 0; args[i] != NULL; i++){
        if(strncmp(args[i], "&", VARIABLE_LENGTH) == 0){
            args[i] = NULL;
            parentWait = false;
            break;
        }else{
            parentWait = true;
        }
    }

    pid_t pid = fork();

    if (pid > 0) {
        addProcess(pid);
        if(parentWait) {
            waitForChild();
        }
    } else if (pid == 0) {
        //Ignore SIGINT and SIGTSTP
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        char pathsToGoTo[VARIABLE_LENGTH];

        //Add CWD and PATH as possible paths to get the executable from
        strncpy(pathsToGoTo, getVariable("CWD"), VARIABLE_LENGTH);
        strncat(pathsToGoTo, ":", VARIABLE_LENGTH);
        strncat(pathsToGoTo, getVariable("PATH"), VARIABLE_LENGTH);

        char* paths[VARIABLE_LENGTH];
        parseStringBy(pathsToGoTo, ":", paths);

        //Go through each path one by one
        for (int i = 0; paths[i] != NULL; i++) {

            char pathtmp[VARIABLE_LENGTH];
            strncpy(pathtmp, paths[i], VARIABLE_LENGTH);
            strncat(pathtmp, "/", VARIABLE_LENGTH);
            strncat(pathtmp, args[0], VARIABLE_LENGTH);
            paths[i] = pathtmp;

            if (readFromPipe){
                dup2(inputPipe[0], STDIN_FILENO);
            }

            if(writeToPipe) {
                dup2(outputPipe[1], STDOUT_FILENO);
            }

            execv(paths[i], args);
        }
        perror("Error");
        exit(-1);
    } else {
        fprintf(stderr, "Fork Failed");
    }
}

void allCommand(){
    for(int i = 0; i < NUMBER_OF_SHELL_VARIABLES; i++){
        printf("%s = %s\n", variables[i]->key, variables[i]->value);
    }
}


void sourceCommand(char* fileName, void recursiveCall(char* line)){
    if(fileName != NULL){
        FILE *fp;

        //Add the CWD to the filename, so that the file in the CWD is opened
        char file_name[VARIABLE_LENGTH];
        strncpy(file_name, getVariable("CWD"), VARIABLE_LENGTH);
        strncat(file_name, "/", VARIABLE_LENGTH);
        strncat(file_name, fileName, VARIABLE_LENGTH);

        //Open file in read mode if it exists
        if (!(fp = fopen (file_name, "r"))){
            fprintf(stderr, "Error, no file found\n");
        }else {
            char ptr[VARIABLE_LENGTH];
            for (int i = 0; fgets(ptr, VARIABLE_LENGTH, fp); i++) {

                ptr[strlen(ptr)-1] = '\0'; //This line is made due to the way gets reads from files

                //The function to compute the lines in the file one by one is called
                recursiveCall(ptr);
            }

            fclose(fp);
        }
    }else {
        fprintf(stderr, "No source file specified\n");
    }
}