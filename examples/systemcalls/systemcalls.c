#include "systemcalls.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <errno.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

    int ret = system(cmd);
    if (ret == -1){
        return false;
    }
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    openlog(NULL,0,LOG_USER);
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];


    // syslog(LOG_DEBUG, "first char of command 0:%d\n", command[0][0]);
    syslog(LOG_DEBUG, "command 0:%s:\n", command[0]);
    // if (command[0][0] != 47) {syslog(LOG_ERR, " %d:\n", errno); return false;}
    int w;
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork"); 
        syslog(LOG_ERR, "fork had trouble! %d\n", errno);
        va_end(args);
        return false;
    }
    else if(pid == 0) {
        int ret = execv(command[0], command); 
        syslog(LOG_DEBUG, "execv error code %d\n", ret);
        if (ret == -1){
        exit(EXIT_FAILURE);
        }
    }
    else {
        wait(&w);
        //syslog(LOG_DEBUG, "wait return code:%d\n", w);
        if (WIFEXITED(w)) {
            syslog(LOG_DEBUG, "wexitstatus: %d",WEXITSTATUS(w));
            if (WEXITSTATUS(w) == 1){
                syslog(LOG_ERR, "returning false");
                va_end(args);
                return false;
            }
        } 
    }
    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    openlog(NULL,0,LOG_USER);
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd < 0) { perror("open"); va_end(args); return false; }
    int w;
    pid_t pid = fork();
    switch (pid){
        case -1: perror("fork"); va_end(args); return false;
        case 0: 
            if (dup2(fd, 1) < 0) { perror("dup2"); exit(EXIT_FAILURE); }
            close(fd);
            int ret = execv(command[0], command); 
            if (ret == -1){
                perror("execv"); 
                exit(EXIT_FAILURE);
            } 
        default: 
            wait(&w);
            if (w != 0) {
                perror("wait");
                va_end(args);
                return false;
            }
            close(fd);
    }
    va_end(args);
    return true;
}
