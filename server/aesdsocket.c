#include "aesdsocket.h"

bool non_interrupted = true;
int fd;
int log_file;

int sock_recv(int log_file, int client_fd){
    char buf[1024];
    memset(buf,0,1024);
    int num_bytes = 1024;
    while (num_bytes == 1024){
        int num_bytes = recv(client_fd, buf, 1024, MSG_DONTWAIT);
        if (num_bytes == -1 && errno == EAGAIN ){
            break;
        }
        else if (num_bytes == -1){
            perror("recv");
            return -1;
        }
        if (num_bytes == 0){
            return 0;
        }
        syslog(LOG_DEBUG, "num_bytes: %d\n",num_bytes); 
        char *pos = (char *)memchr(buf,'\n',sizeof(buf));
        int delim = 0;
        if (pos != NULL){ // \n in string, so do stuff
            delim = strcspn(buf,"\n");
        } else {
            delim = num_bytes -1; // 1023
        }
        int rc = write(log_file, buf, delim+1);
        if (rc == -1){
            perror("write");
            return -1;
        }
        syslog(LOG_DEBUG, "rc: %d, log_file: %d\n",rc, log_file);    
        syslog(LOG_DEBUG, "Wrote %d bytes to file.\n", delim+1);
    }

    return 0;
}

int sock_send(int log_file, int client_fd){
    char buf[1024];
    memset(buf,0,1024);
    int read_count = 1024;
    lseek(log_file,0,SEEK_SET);
    while (read_count > 0){
        int read_count = read(log_file, buf, sizeof(buf)-1);
        syslog(LOG_DEBUG, "read_count: %d, log_file: %d, client_fd: %d\n",read_count, log_file, client_fd);
        if (read_count == -1){
            perror("read");
            return -1;
        } 
        if (read_count == 0) {
            return 0;
        }
        syslog(LOG_DEBUG, "Read %d bytes from log file.\n", read_count);

        int sent_bytes = send(client_fd, buf, read_count,0);
        if (sent_bytes == -1){
            perror("send");
            return -1;
        }
        syslog(LOG_DEBUG, "Sent %d bytes back.\n", sent_bytes);
    }
    return 0;
}

void signal_handler(int signal_number){
    if (signal_number == SIGINT) {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        non_interrupted = false;
        shutdown(fd, SHUT_RDWR);
        if (unlink("/var/tmp/aesdsocketdata") == -1){
            syslog(LOG_ERR, "Could not unlink /var/tmp/aesdsocketdata\n");
        } else {
            syslog(LOG_DEBUG, "Unlinked /var/tmp/aesdsocketdata\n");
        }
        if (close(log_file) == -1){
            syslog(LOG_ERR, "Could not close /var/tmp/aesdsocketdata\n");
        }
    } else if (signal_number == SIGTERM) {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        non_interrupted = false;
        shutdown(fd, SHUT_RDWR);
        if (unlink("/var/tmp/aesdsocketdata") == -1){
            syslog(LOG_ERR, "Could not unlink /var/tmp/aesdsocketdata\n");
        } else {
            syslog(LOG_DEBUG, "Unlinked /var/tmp/aesdsocketdata\n");
        }
        if (close(log_file) == -1){
            syslog(LOG_ERR, "Could not close /var/tmp/aesdsocketdata\n");
        }
    }
}

int perform_socket_actions(int log_file, int listen_backlog) {
    struct sockaddr_in client_addr;
    socklen_t client_len;
    client_len = sizeof(client_addr);

    while (non_interrupted == true){
        if (listen(fd,listen_backlog) == -1){
            perror("listen");
            syslog(LOG_ERR, "Could not listen on socket %d!\n", fd);
            close(fd);
            return -1;
        } else {
            syslog(LOG_DEBUG, "Listening on socket %d\n", fd);

        }

        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            continue;
        } else if (client_fd == -1 && errno == EINTR){
            close(fd);
            break;
        }else if (client_fd == -1){
            close(fd);
            break;
        } else {
            syslog(LOG_DEBUG, "Accepted connection from %d\n", client_fd);
        }
        
        int rc = sock_recv(log_file, client_fd);
        if (rc == -1){
            syslog(LOG_ERR, "Could not receive from %d\n", client_fd);
        }
        
        rc = sock_send(log_file, client_fd);
        if (rc == -1){
            syslog(LOG_ERR, "Could not send to %d\n", client_fd);
        }
        close(client_fd);  
    }

    return 0;

}

int main(int argc, char *argv[]){
    openlog(NULL,0,LOG_USER);
    int listen_backlog = 100;

    struct sigaction new_action;
    memset(&new_action,0,sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;

    if (sigaction(SIGTERM, &new_action, NULL) == -1) {
            perror("sigaction");
            return -1;
    }

    if (sigaction(SIGINT, &new_action, NULL) == -1) {
            perror("sigaction");
            return -1;
    }

    log_file = open("/var/tmp/aesdsocketdata", O_CREAT | O_RDWR | O_NONBLOCK, 0777);
    if (log_file == -1) {
        perror("open");
        syslog(LOG_ERR, "Could not access /var/tmp/aesdsocketdata\n");
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 

    fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (fd == -1) {
        syslog(LOG_ERR, "Could not create socket!\n");
        return -1;
    }
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1 ){
        perror("setsockopt");
    }


    int status = getaddrinfo(NULL, "9000", &hints, &servinfo);
    if (status != 0){
        perror("getaddrinfo");
        syslog(LOG_ERR, "Could not get address info!\n");

        return -1;
    }
    
    int rc = bind(fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (rc == -1){
        perror("bind");
        syslog(LOG_ERR, "Could not bind socket address!\n");
        close(fd);
        return -1;
    }

    freeaddrinfo(servinfo);

    pid_t pid;
    if (argc == 2){
        if (strcmp(argv[1], "-d") == 0){
            syslog(LOG_DEBUG, "Deamon flag set, forking");
            pid = fork();
            switch (pid) {
            case -1:
                perror("fork");
                return -1;
            case 0:
                return perform_socket_actions(log_file, listen_backlog);
            default:
                return 0; // parent does nothing in this case.
            }
        } 
    }
    else {
        return perform_socket_actions(log_file, listen_backlog);
    }

}