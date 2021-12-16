/*
 * Imprimer: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "imprimer.h"
#include "conversions.h"
#include "sf_readline.h"
#include "helper.h"


volatile sig_atomic_t pid;

void SIGCHLDHandler(int sig){
    int olderrno = errno;
    while(waitpid(-1, NULL, 0) > 0){
        perror("Handler reaped child\n");
    }
    if(errno != ECHILD) perror("waitpid error");
    //pid = waitpid(-1, NULL, 0); /* main is waiting for nonzero pid */
    errno = olderrno;
} //need to add more to this ??
// blocked signal --> if received, it's swallowed
//worth checking error code and using error is there is a failure

int run_cli(FILE *in, FILE *out)
{
    //sigset_t mask_all, mask_one, prev_one;
    //signal(SIGCHLD, SIGCHLDHandler);
    //int pid = fork();
    //if(pid){

    //}
    // TO BE IMPLEMENTED
    //printf("here3.\n");
    int flag = 1;
    char * input = NULL;
    char * line = NULL;
    size_t length = 0;
    ssize_t read = 0;
    int counter = 0;

    if(in != stdin){
        line = (char *)malloc(255);
        while(((read = getline(&line, &length, in)) != -1) && (flag !=-1)){
            flag = parse_arguments(in, line, out);
            counter++;
            if(flag == 2){
                free(line);            }
            else if(flag == 1){
                flag = -1;
                free(line);
                free_printers();
                free_jobs();
                return 1;
            } else {
                free(line);
            }
            line = (char *)malloc(255);
        }
        free(line);
    }
    //free(line);

    if(read == -1){
        //free(line);
        return 0;
    }

    //printf("flag: %d\n", flag);

    if(counter == 0 && in != stdin){
        flag = 0;
    }else if(flag != -1){

        flag = 1;
        //printf("hello3\n");
    }
    else {
        return 1;
    }

    //when forking, need to call other function likely and then set up conversion pipeline
    //set up signal handler

    while(flag){
        //printf("ugh\n");
        if(out != stdout)
            input = sf_readline("");
        else
           input = sf_readline(PROMPT);
        int x = parse_arguments(in, input, out);
        if(x == 2){
            free(input);
            continue;
        } else if(x == 1){
            free(input);
            flag = 0;
        } else {
            free(input);
        }
    }

    free_printers();
    free_jobs();
    return 0;
}
