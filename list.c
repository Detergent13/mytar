#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"
#include "header.h"

#define OCTAL 8
#define BLOCKSIZE 512
#define MAX_NAME_LEN 255
#define EMPTY_BLOCK_CHKSUM 256

extern int errno;

int list_cmd(char* fileName, char *directories[], int numDirectories, int verboseBool, int strictBool);

/* This should be removed for final product. Just using this for testing! */
int main() {
    list_cmd("../test.tar", NULL, 0, 1, 0);
}

int list_cmd(char* fileName, char *directories[], int numDirectories, int verboseBool, int strictBool) {
    int fd;
    struct header headerBuffer;
     
    /* Validate if fileName is a .tar */
    if(!strstr(fileName, ".tar\0")) {
        fprintf(stderr, "Passed file is not a .tar!");
        exit(EXIT_FAILURE);
    }

    /* Open the tarfile and exit on any errors */
    errno = 0;
    fd = open(fileName, O_RDONLY);

    if(errno) {
        fprintf(stderr, "Couldn't open file %s: %s",
            fileName, strerror(errno));
        exit(errno);
    }

    /* TODO: Think of an elegant way to catch errno for each iteration - 0, loop, check, body, 0*/
    /* TODO: Check for end block (should fix 'ghost headers') - if the checksum is just spaces, skip over! */
    /* TODO: Print 'real names' if symbolic name unavailable */
    /* TODO: Handle symlinks- is this any different? */
    /* TODO: Handle specified paths */
    /* TODO: Validate checksum */
    while(read(fd, &headerBuffer, sizeof(struct header)) > 0) {
        char *fullName;
        int i;
        /* Read the size of the file */
        unsigned long int fileSize = strtol(headerBuffer.size, NULL, OCTAL);
        int expectedChecksum = calc_checksum(&headerBuffer);
        int readChecksum = strtol(headerBuffer.chksum, NULL, OCTAL);

        if(expectedChecksum != readChecksum) {
            fprintf(stderr, "Expected checksum %d doesn't match read checksum %d\n",
                    expectedChecksum, readChecksum);
            exit(EXIT_FAILURE);
        }

        
        errno = 0;
        /* +2 to make space for slash and null-term */
        fullName = calloc(MAX_NAME_LEN + 2, sizeof(char));
        if(errno) {
            perror("Couldn't calloc fullName");
        }
        
        /* Check if prefix exists */
        /* We're using the alternative 'n' methods here to avoid undefined
         * behaviour if either the prefix or name isn't null terminated.
         * (Which is entirely possible with the standard) */
        if(headerBuffer.prefix[0]) {
            strncpy(fullName, &headerBuffer.prefix, sizeof(headerBuffer.prefix));
            strcat(fullName, "/");
            strncat(fullName, &headerBuffer.name, sizeof(headerBuffer.name));
        }
        else {
            strncpy(fullName, &headerBuffer.name, sizeof(headerBuffer.name));
        }

        if(directories) {
            /*for(int i = 0; i < numDirectories; */
        }

        if(!verboseBool) {
            printf("%s\n", fullName); 
        }
        else {
            printf("File: %s%s, size: %ld\n", &headerBuffer.prefix, &headerBuffer.name, fileSize);
        }

        /* Skip over the body to next header */
        if(fileSize > 0) {
            /* If the file has size > 0, skip ahead by the required # blocks */
            lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE, SEEK_CUR);
        }
    }
    return 0;
}
