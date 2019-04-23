#include <stdbool.h>
#include <sys/wait.h>

void printCommand(char** args);
void chdirCommand(char* newDir);
void variableModificationCommand(char* line);
void forkChild(char** args, int* inputPipe, int* outputPipe, bool readFromPipe, bool writeToPipe);
void allCommand();
void sourceCommand(char* filename, void recursiveCall(char* line));