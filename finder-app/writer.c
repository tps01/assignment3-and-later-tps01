#include <errno.h>
#include <stdio.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
    openlog(NULL,0,LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "A filename and a string to write must be provided as arguments!");
        return 1;
    }

    char *writefile = argv[1];
    char *writestr = argv[2];

    FILE *file;
    file = fopen(writefile, "w");
    if (file == NULL) {
        perror("Unexpected error occurred.");
        syslog(LOG_ERR, "Error opening file %s: %d\n", writefile, errno);
        return 1;
    } else {
        syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
        fputs(writestr,file);
    }

    fclose(file);
    closelog();
    return 0;
}