#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    openlog(NULL,0,LOG_USER);
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->wait_to_obtain_ms*1000);
    syslog(LOG_DEBUG, "wait_to_obtain_ms: %d",thread_func_args->wait_to_obtain_ms);
    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0) {
        thread_func_args->thread_complete_success = false;
        pthread_exit(thread_func_args);
    }
    syslog(LOG_DEBUG, "lock mutex status: %d",rc);
    usleep(thread_func_args->wait_to_release_ms*1000);
    syslog(LOG_DEBUG, "wait_to_release_ms: %d",thread_func_args->wait_to_release_ms);
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    syslog(LOG_DEBUG, "unlock mutex status: %d",rc);
    thread_func_args->thread_complete_success = true;
    return thread_func_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    openlog(NULL,0,LOG_USER);
    struct thread_data* thread_func_args = (struct thread_data *)malloc(sizeof(struct thread_data));
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    int success = pthread_create(thread, NULL, threadfunc, thread_func_args);
    syslog(LOG_DEBUG, "thread success status: %d",success);
    if (success == 0){
        return true;
    } else {
        perror("pthread_create");
        free(thread_func_args);
        return false;
    }
}

