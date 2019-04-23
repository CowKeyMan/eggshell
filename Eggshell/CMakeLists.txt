cmake_minimum_required(VERSION 3.9)
project(AssignmentRedone C)

set(CMAKE_C_STANDARD 99)

add_executable(eggshell variableManager.c parser.c linenoise.c eggshell.c commandsManager.c processManager.c)