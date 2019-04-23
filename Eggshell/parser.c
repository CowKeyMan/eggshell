#include "parser.h"

//Changes the contents of returnArgs to the string parsed by the delimeters parseChar
void parseStringBy(char *string, char* parseString, char** returnArgs) {
    char *token;
    token = strtok(string, parseString);

    int tokenIndex = 0;
    while (token != NULL && tokenIndex < MAX_ARGUMENTS) {
        returnArgs[tokenIndex] = token;
        token = strtok(NULL, parseString);
        tokenIndex++;
    }

    //Set last returnArgs as NULL
    returnArgs[tokenIndex] = NULL;
}

//Parser with an entire string as delimeter
void parseStringByString(char *string, char* parseString, char** returnArgs) {
    //Copy string into a buffer
    char stringBuf[VARIABLE_LENGTH];
    strncpy(stringBuf, string, VARIABLE_LENGTH);

    //Create a bufer for the returnArgs
    char returnArgsBuf[MAX_ARGUMENTS][VARIABLE_LENGTH];
    returnArgsBuf[0][0] = '\0';

    int currentString = 0; //The current string being added to
    int currentStringIndex = 0; //The position in the string

    //current char is the current character in the string being parsed
    for(int currentChar = 0; currentChar < strlen(stringBuf); currentChar++, currentStringIndex++){
        //Add current character to the argument
        returnArgsBuf[currentString][currentStringIndex] = stringBuf[currentChar];

        //go through the string from the current position, until the string does not match the parsing string
        for(int i2 = 0; i2+currentChar < strlen(stringBuf) && stringBuf[i2+currentChar] == parseString[i2]; i2++){
            //If the string is found, then parse it at this point
            if(i2 == strlen(parseString)-1){
                if(returnArgsBuf[currentString][0] != '\0'){
                    returnArgsBuf[currentString][currentStringIndex] = '\0';
                    currentString++;
                    returnArgsBuf[currentString][0] = '\0';
                    currentStringIndex = -1;
                }
                currentChar+=i2;
                continue;
            }
        }
    }

    returnArgsBuf[currentString][currentStringIndex] = '\0';
    //Set final arg as null
    returnArgsBuf[currentString+1][0] = '\0';



    for(int i = 0; returnArgsBuf[i][0] != '\0'; i++){
        removeSpaces(returnArgsBuf[i]);
        returnArgs[i] = returnArgsBuf[i];
    }
}