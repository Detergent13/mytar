#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include "util.h"
#include "header.h"

#define OCTAL 8
#define BLOCKSIZE 512
#define REG_FLAG '0'
#define REG_FLAG_ALT '\0'
#define SYM_FLAG '2'
#define DIR_FLAG '5'

int extract_cmd(char* fileName, int verboseBool, int strictBool) {
    int fd;
    struct header headerBuffer;

    errno = 0;
    fd = open(fileName, O_RDONLY);
    
    if(errno) {
        fprintf(stderr, "Couldn't open file %s: %s",
            fileName, strerror(errno));
        exit(errno);
    }

    errno = 0;
    while(read(fd, &headerBuffer, sizeof(struct header)) > 0) {
        unsigned long int fileSize;
        unsigned char typeFlag = *headerBuffer.typeflag;
        struct stat statBuffer;
        struct utimbuf newTime;
        char* createdPath;

        /* Check read() error */
        if(errno) {
            perror("Couldn't read header");
            exit(errno);
        }

        fileSize = strtol(headerBuffer.size, NULL, OCTAL);

        /* TODO: When we create the file, put its path into createdPath */
        /* Make sure it's either an absolute path or a valid relative path (starts with ./) */
        /* TODO: Restore permissions (if applicable?) */
        switch (typeFlag) {
            case REG_FLAG_ALT:
            case REG_FLAG:
                /* case for regular file (create) */
                break;
            case SYM_FLAG:
                /* case for symlink (symlink(..., ...)) */
                break;
            case DIR_FLAG:
                /* case for directory (mkdir, doesn't matter if it's already there */
                break;
            default:
                fprintf(stderr, "Invalid typeflag! '%c'", typeFlag);
                exit(EXIT_FAILURE);
        }
       
        
        /* Stat the created file to get its current times */
        errno = 0;
        if(stat(createdPath, &statBuffer)) {
            perror("Failed to stat created file!");
            exit(errno);
        } 

        /* Preserve atime */
        newTime.actime = statBuffer.st_atime;
        /* Set mtime to mtime from header */
        newTime.modtime = strtol(headerBuffer.mtime, NULL, OCTAL);
        /* Actually write the time */
        utime(createdPath, &newTime);

        /* NOTE: We shouldn't be touching the created file after this.
         * The mtime could be disturbed. Do any operations on it before 
         * setting mtime. */

        /* Skip over the body to next header */
        if(fileSize > 0) {
            /* If the file has size > 0, skip ahead by the required # blocks */
            if(lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE, SEEK_CUR) == -1) {
                perror("Couldn't lseek to next header");
                exit(errno);
            }
        } 

        errno = 0;
    }
    close(fd);
    return 0;
}
