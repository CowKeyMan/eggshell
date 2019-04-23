#include "processManager.h"
#include "variableManager.h"

void addProcess(int newPID){
    processAmount++;
    processes = realloc(processes, processAmount * sizeof(int));
    processes[processAmount-1] = newPID;
}
int getLastProcess(){
    return processes[processAmount-1];
}
void removeLastProcess(){
    processAmount--;
    processes = realloc(processes, processAmount * sizeof(int));
}

void waitForChild(){
    int status;
    waitpid(getLastProcess(), &status, WUNTRACED);
    char c[8];
    sprintf(c, "%d", status);
    setVariable("EXITCODE", c);
    if(kill(getLastProcess(), 0) != 0){
        removeLastProcess();
    }
}

void removePID(int pid){
    for(int i = 0; i < processAmount; i++){
        if(processes[i] == pid){
            while(i < processAmount){
                processes[i] = processes[i+1];
                i++;
            }
            removeLastProcess();
            return;
        }
    }
}