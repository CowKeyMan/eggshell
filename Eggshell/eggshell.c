#include "commandsManager.h"
#include "linenoise.h"
#include "parser.h"
#include "processManager.h"

void computeLine(char* line);
char* parseForOutputRedirection(char* line);
char* parseForInputReceival(char* line);
void parseForPiping(char* line);
void performCommand(char* line);
void initialiseShellVariables(char* zerothArgument);
void initialiseProcessesAndSignlalHandling();
void resetItemsAtEndOfCommand();
void resetIO();

void signalHandler(int signalNumber);

//Signal handler struct
struct sigaction sa;

//Used to store the default STDIN, OUT, ERR for reverting them after a command
int stdinFileDescriptor;
int stdoutFileDescriptor;
int stderrFileDescriptor;

//used for output and input redirection
FILE* outputFile;
FILE* inputFile;

//Only 2 pipes are used
int pipe1[2];
int pipe2[2];

//Bool to indicate wether child process should read/write
//Like pipes, 0 = read and 1 = write
bool IOint[2];

//An alternating bool to indicate which pipe is out/input, when we have multiple pipes
bool currentPipe = 0;

//A bool to indicate wether the parent should wait or not after forking a child
bool parentWait = true;

int main(int argc, char *argv[]){
    //At the start, set IOint both to 0 (false)
    resetIO();

    //Duplicate the default file descriptors
    stdinFileDescriptor = dup(STDIN_FILENO);
    stdoutFileDescriptor = dup(STDOUT_FILENO);
    stderrFileDescriptor = dup(STDERR_FILENO);

    //Initialise items
    initialiseShellVariables(argv[0]);
    initialiseProcessesAndSignlalHandling();

    linenoiseHistorySetMaxLen(16);

    //This loop terminates only when the exit function is used in the eggshell
    char* line;
    while(true) {
        line = linenoise(getVariable("PROMPT"));

        //Error prevention, otherwise a seg.fault is caused at CTRL+C during linenoise
        if(line == NULL){
            continue;
        }

        //Check for empty line or line with just spaces, in which case loop through again
        char _line[VARIABLE_LENGTH];
        strncpy(_line, line, VARIABLE_LENGTH);
        removeSpaces(_line);

        if(strlen(_line) <= 0){
            continue;
        }

        linenoiseHistoryAdd(line);

        //Otherwise, work on the line
        computeLine(line);

        //Free line for reuse
        linenoiseFree(line);
    }
}

//Method to set up shell variable related components
void initialiseShellVariables(char* zerothArgument){
    variableAmount = 0;
    variables = malloc(0);
    setVariable("PATH", "/usr/bin:/bin:/usr/local/bin");
    setVariable("PROMPT", "> ");
    setVariable("CWD", "");
    setVariable("USER", "usr");
    setVariable("HOME", "/home");
    setVariable("SHELL", "");
    setVariable("TERMINAL", "/usr/bin");
    setVariable("EXITCODE", "0");

    setUpShellVariables(zerothArgument);
}


void initialiseProcessesAndSignlalHandling(){
    processAmount = 0;

    processes = malloc(0);
    sa.sa_handler = signalHandler;

    //Handle SIGINT, SIGCONT and SIGCHLD
    if(sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTSTP, &sa, NULL) == -1 || sigaction(SIGCHLD, &sa, NULL) == -1){
        fprintf(stderr, "ERROR, signals not catchable");
    }
}

//Line is parsed in multiple ways before checking for commands
void computeLine(char* line){
    //First we check if an output file is specified
    line = parseForOutputRedirection(line);
    //Then we check if an input file/herestring is specified
    line = parseForInputReceival(line);

    //Then we check for piping and execute items accordingly
    parseForPiping(line);

    //At the end we reset and go back to linenoise for next command
    resetItemsAtEndOfCommand();
}

char* parseForOutputRedirection(char* line){
    //The args to be returned
    char* args[2];

    //First check if there will be any appending
    parseStringByString(line, ">>", args);
    if(args[1] != NULL && args[1][0] != '\0'){ //Ensuring no seg fault occurs
        //Open the outputFile for appending and redirect STDOUT and STDERR to it
        if(outputFile = fopen(args[1], "a")) {
            dup2(fileno(outputFile), STDOUT_FILENO);
            dup2(fileno(outputFile), STDERR_FILENO);
        }else{
            perror("Error: ");
        }
    }
    else{
        //Check if there will be writing to file
        parseStringByString(line, ">", args);
        if(args[1] != NULL && args[1][0] != '\0'){
            //Open the outputFile for writing and redirect STDOUT and STDERR to it
            if(outputFile = fopen(args[1], "w")){
                dup2(fileno(outputFile), STDOUT_FILENO);
                dup2(fileno(outputFile), STDERR_FILENO);
            }else{
                perror("Error: ");
            }
        }

    }
    //return the command without the output items
    //Ex: If the command is 'ls > out.txt', 'ls ' will be returned
    return args[0];
}

char* parseForInputReceival(char* line){
    char* args[2];

    //Check for 'here string'
    parseStringByString(line, "<<<", args);
    if(args[1] != NULL && args[1][0] != '\0') {
        //In the case of' here string', write it to a temporary file, and et that temporary file as stdin to the command
        inputFile = tmpfile();
        fprintf(inputFile, "%s", args[1]);
        rewind(inputFile);
        dup2(fileno(inputFile), STDIN_FILENO);
    }
    else{
        //Parse string for input file
        parseStringBy(line, "<", args);
        if(args[1] != NULL){
            //In the case of input file, redirect stdin to it
            removeSpaces(args[1]);
            if(inputFile = fopen(args[1], "r")) {
                dup2(fileno(inputFile), STDIN_FILENO);
            }
        }
    }
    return args[0];
}

void parseForPiping(char* line){
    //fixed array to hold each command, ex: "ls -f", "figlet"
    char* args[MAX_ARGUMENTS];

    parseStringBy(line, "|", args);

    int numberOfArgs = 0;
    for(int i = 0; args[i] != NULL; i++){
        numberOfArgs++;
    }

    //Loop through each argument
    for(int i = 0; i < numberOfArgs; i++){

        //If it is the first argument and it needs to output to a pipe
        if(i == 0 && numberOfArgs > 0) {
            pipe(pipe1);
        }

        //If it is not the first command
        if(i > 0) {
            //Set receive from pipe to true
            IOint[0] = 1;

            //Set a pipe for output redirection, so close the previous pipe's write
            if (currentPipe) {
                pipe(pipe2);
                close(pipe1[1]);
            } else {
                pipe(pipe1);
                close(pipe2[1]);
            }
        }
        if(i < numberOfArgs-1){
            //If it is not the last command, set output to the pipe
            IOint[1] = 1;
        }

        //Do the current command
        performCommand(args[i]);

        //Reset IOint
        resetIO();

        //Close the read end of the previous pipe
        if (currentPipe) {
            close(pipe2[0]);
        } else {
            close(pipe1[0]);
        }
    }

    //At the end, close all open pipes
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    //Reset items like STDOUT ad STDIN at the end only
    resetItemsAtEndOfCommand();
}

void performCommand(char* line) {
    //First split the arguments, and put them into an array
    char* args[MAX_ARGUMENTS];
    parseStringBy(line, " ", args);

    //Check for each command by comparing the first argument
    //Then execute the respective command from commandManager
    if (strcmp(args[0], "exit") == 0){
        exit(0);
    }else if (strcmp(args[0], "source") == 0){
        sourceCommand(args[1], &computeLine);
    }
    else if (strcmp(args[0], "chdir") == 0) {
        chdirCommand(args[1]);
    }
    else if (strcmp(args[0], "print") == 0) {
        printCommand(args);
    }
    else if (strcmp(args[0], "all") == 0) {
        allCommand();
    }
    else if (strcmp(args[0], "bg") == 0) {
        //For bg, do not wait for the child
        if(processAmount > 0) {
            kill(getLastProcess(), SIGCONT);
        }else{
            fprintf(stderr, "No current processes\n");
        }
    }
    else if (strcmp(args[0], "fg") == 0) {
        //For fg, wait for the child
        if(processAmount > 0) {
            kill(getLastProcess(), SIGCONT);
            pause();
            waitForChild();
        }else{
            fprintf(stderr, "No current processes\n");
        }
    }
    else if (strcmp(args[0], "jobs") == 0) {
        printf("%d process/es on stack\n", processAmount);
    }
    //if a command to change/add a variable is entered, indicated by the first character being $
    else if (args[0][0] == '$') {
        char fullLine[VARIABLE_LENGTH];
        //For this, concatinate the entire given line, so string it back together
        strncpy(fullLine, args[0], VARIABLE_LENGTH);
        for(int i = 1; args[i] != NULL; i++){
            strncat(fullLine, args[i], VARIABLE_LENGTH);
        }

        variableModificationCommand(fullLine);
    }

    //when an external command is entered
    else {
        //Alternate the pipes
        if(currentPipe) {
            forkChild(args, pipe1, pipe2, IOint[0], IOint[1]);
        }else{
            forkChild(args, pipe2, pipe1, IOint[0], IOint[1]);
        }
        currentPipe = !currentPipe;
    }
}

void resetItemsAtEndOfCommand(){
    //Reset file descriptors
    dup2(stdinFileDescriptor, 0);
    dup2(stdoutFileDescriptor, 1);
    dup2(stderrFileDescriptor, 2);

    //Close any open files
    if(outputFile != NULL){
        fclose(outputFile);
        outputFile = NULL;
    }
    if(inputFile != NULL){
        fclose(inputFile);
        inputFile = NULL;
    }

    //Reset IOint
    resetIO();

    //Reset this bool, so that the first pipe will always be pipe1
    currentPipe = 0;
}


void resetIO(){
    IOint[0] = IOint[1]= 0;
}

void signalHandler(int signalNumber){
    //In the case the child is interrupted externally
    if(signalNumber == SIGCHLD){
        int status;
        int i = waitpid(-1, &status, WNOHANG);
        if(i > 0 && kill(i, 0) != 0) {
            printf("Child aborted\n");
            removePID(i);
        }
    }

    //Send signals to child to Interrupt or Stop
    if (signalNumber == SIGINT) {
        printf("Process Terminated\n");
        kill(getLastProcess(), SIGKILL);
    }
    else if (signalNumber == SIGTSTP) {
        printf("Process Stopped\n");
        kill(getLastProcess(), SIGSTOP);
    }
}