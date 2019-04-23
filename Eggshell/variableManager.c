#include "variableManager.h"

//Method to create a variable or edit its value
void setVariable(char* varName, char* newCharValue){
    //Search trough the variables and if the key is found, change the character's value
    for(int i = 0; i < variableAmount; i++){
        if(strncmp(variables[i]->key, varName, VARIABLE_LENGTH) == 0){
            strncpy(variables[i]->value, newCharValue, VARIABLE_LENGTH);
            return;
        }
    }
    //Otherwise add a new variable
    variableAmount++;
    variables = realloc(variables, variableAmount * sizeof(struct Variables**));
    variables[variableAmount-1] = malloc(sizeof(struct Variables));
    strncpy(variables[variableAmount-1]->key, varName, VARIABLE_LENGTH);
    strncpy(variables[variableAmount-1]->value, newCharValue, VARIABLE_LENGTH);
}

//Get the value of a variable, given its name as a parameter
char* getVariable(char* varName){
    for(int i = 0; i < variableAmount; i++){
        if(strncmp(variables[i]->key, varName, VARIABLE_LENGTH) == 0){
            return variables[i]->value;
        }
    }
    //If the variable name is not found, return NULL
    return NULL;
}

//Method to be called at the start of the class, to set up the variables: CWD, SHELL and TERMINAL
void setUpShellVariables(char* shellNameWithDotSlash){
    //Making sure CWD is set properly
    char cwdbuf[VARIABLE_LENGTH];
    if(getcwd(cwdbuf, VARIABLE_LENGTH) == NULL) {
        perror("Error getting working directory: ");
        exit(-1);
    }else{
        setVariable("CWD", cwdbuf);
        //Set up shell variable
        char shellName[VARIABLE_LENGTH];
        strncpy(shellName, getVariable("CWD"), VARIABLE_LENGTH);
        //Trim the first letter since this is a '.' (ex: ./eggshell)
        strncat(shellName, trimFirstLetter(shellNameWithDotSlash), VARIABLE_LENGTH);
        setVariable("SHELL", shellName);
    }

    //Making sure TERMINAL is set properly
    if(ttyname(STDIN_FILENO) == NULL){
        perror("Error getting ttyname: ");
        exit(atoi(getVariable("EXITCODE")));
    }else{
        setVariable("TERMINAL", ttyname(STDIN_FILENO));
    }
}

//remove the first letter from a given character and return it
char* trimFirstLetter(char* string){
    char buf[VARIABLE_LENGTH];
    for (int j = 0; string[j] != '\0' && string[j] != '\n'; j++) {
        buf[j] = string[j + 1];
    }
    char* ret = buf;
    return ret;
}

//Method to remove spaces from the beginning and end of the word
void removeSpaces(char* string){
    char buf[VARIABLE_LENGTH];

    //Traverse the string, ignoring spaces in the beginning
    int counter = 0;
    int j = 0;
    while(string[j] == ' '){
        j++;
    }

    while (j < strlen(string) && j<VARIABLE_LENGTH) {
        buf[counter] = string[j];
        counter++;
        j++;
    }

    //Then set spaces at the end of the string as '\0'
    buf[counter] = '\0';
    for (int j2 = counter-1; j2 >= 0; j2--) {
        if(buf[j2] == ' '){
            buf[j2] = '\0';
        }else{
            break;
        }
    }
    //At the end, change the string to the buffer
    strncpy(string, buf, VARIABLE_LENGTH);
}