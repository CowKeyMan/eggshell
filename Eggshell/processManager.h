#include <stdlib.h>
#include <wait.h>

int* processes;
int processAmount;

void addProcess(int newPID);
int getLastProcess();
void removeLastProcess();
void removePID(int pid);
void waitForChild();