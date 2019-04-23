#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_ARGUMENTS 64
#define VARIABLE_LENGTH 512
#define NUMBER_OF_SHELL_VARIABLES 8

struct Variables{
    char key[VARIABLE_LENGTH], value[VARIABLE_LENGTH];
};

struct Variables** variables;
int variableAmount;

void setVariable(char* varName, char* newCharValue);
char* getVariable(char* varName);

void setUpShellVariables(char* shellNameWithDotSlash);

void removeSpaces(char* string);
char* trimFirstLetter(char* string);