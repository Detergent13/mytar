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
#define MAGIC_LEN 6
#define VERSION_LEN 2
/* equivalent to 100 000 000 */
#define STARTING_MASK 256
#define PERMS_LEN 9
#define REG_FLAG '0'
#define REG_FLAG_ALT '\0'
#define SYM_FLAG '2'
#define DIR_FLAG '5'
#define OWNER_LEN 17
#define SPECIAL_INT_MASK 0x800000

extern int errno;

int list_cmd(char* fileName, char *directories[], int numDirectories, int verboseBool, int strictBool);

/* This should be removed for final product. Just using this for testing! */
int main() {
    char *dirs[] = {"asgn4/.git/objects/e6/", "asgn4/.git/objects/9b/"};
    list_cmd("../test.tar", NULL, 0, 1, 0);
    return 0;
}

int list_cmd(char* fileName, char *directories[], int numDirectories, int verboseBool, int strictBool) {
    int fd;
    struct header headerBuffer;
     
    /* Validate if fileName is a .tar */
    if(!strstr(fileName, ".tar")) {
        fprintf(stderr, "Passed file is not a .tar!\n");
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

    /* TODO: Print 'real names' if symbolic name unavailable */
    /* TODO: Handle symlinks- is this any different? */
    /* TODO: Parse info */
    /* Perms, Owner/Group, Size, Mtime, Name */
    errno = 0;
    while(read(fd, &headerBuffer, sizeof(struct header)) > 0) {
        char *fullName;
        int i, expectedChecksum, readChecksum;
        unsigned long int fileSize;
        char ownerGroup[OWNER_LEN + 1];
        char perms[] = "drwxrwxrwx";
        int mask = STARTING_MASK;
        int readMode = strtol(headerBuffer.mode, NULL, OCTAL);

        /* Check read() error */
        if(errno) {
            perror("Couldn't read header");
            exit(errno);
        }

        fileSize = strtol(headerBuffer.size, NULL, OCTAL);
        expectedChecksum = calc_checksum(&headerBuffer);
        readChecksum = strtol(headerBuffer.chksum, NULL, OCTAL);
   
        /* Skip over any fully empty blocks (aka the end padding) */ 
        if(readChecksum == 0 && expectedChecksum == EMPTY_BLOCK_CHKSUM) {
            if(fileSize > 0) {
                /* If the file has size > 0, skip ahead by the required # blocks */
                if(lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE, SEEK_CUR) == -1) {
                    perror("Couldn't lseek to next header");
                    exit(errno);
                }
            }
            continue;
        }
    
        /* Validate checksum */
        if(expectedChecksum != readChecksum) {
            fprintf(stderr, "Expected checksum %d doesn't match read checksum %d\n",
                    expectedChecksum, readChecksum);
            exit(EXIT_FAILURE);
        }

        /* Check for magic string - minus one since strncmp stops on null */
        if(strncmp("ustar", headerBuffer.magic, MAGIC_LEN - 1) != 0) {
            fprintf(stderr, "Magic string doesn't check out: \"%s\"\n",
                (char *)&headerBuffer.magic);
            exit(EXIT_FAILURE);
        }

        /* Check for version, IF in strict mode */
        if(strictBool && strncmp("00", headerBuffer.version, VERSION_LEN) != 0) {
            /* The .2 limits the # chars we print */
            fprintf(stderr, "Version doesn't check out: \"%.2s\"\n",
                (char *)&headerBuffer.version);
            exit(EXIT_FAILURE);
        }       

        /* Mask away the 'd' if it's not a directory */
        if(*(headerBuffer.typeflag) != DIR_FLAG) {
            *perms = '-';
        } 

        /* Check each perms bit and set accordingly */
        for(i = 0; i < PERMS_LEN; i++) {
            if(!(mask & readMode)) {
                perms[i + 1] = '-';
            }
            mask >>= 1;
        }

        if(headerBuffer.uname[0]) {
            char group[9]; /* magic number, careful */
            sprintf((char *)&ownerGroup, "%.8s", (char *)&headerBuffer.uname);
            strcat((char *)&ownerGroup, "/");
            sprintf((char *)&group, "%.8s", (char *)&headerBuffer.gname);
            strcat((char *)&ownerGroup, (char *)&group);
        }
        else {
            int uid = strtol(headerBuffer.uid, NULL, OCTAL);
            if(uid & SPECIAL_INT_MASK) {
                /* extract special ints here */
            }
            else {
                /* strtol as usual and concat */
            }
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
            strncpy(fullName, (char *)&headerBuffer.prefix, sizeof(headerBuffer.prefix));
            strcat(fullName, "/");
            strncat(fullName, (char *)&headerBuffer.name, sizeof(headerBuffer.name));
        }
        else {
            strncpy(fullName, (char *)&headerBuffer.name, sizeof(headerBuffer.name));
        }

        /* If we've passed a non-null directories[], check all directories
         * against the beginning of current fullName and don't print if
         * it doesn't match an element of directories[]. */
        if(directories) {
            int inDirectoriesBool = 0;
            for(i = 0; i < numDirectories; i++) {
                int pathLength = strlen(directories[i]);
                if(strncmp(fullName, directories[i], pathLength) == 0) {
                    inDirectoriesBool = 1;
                    break;
                }
            }
            if(!inDirectoriesBool) {
                if(fileSize > 0) {
                    /* If the file has size > 0, skip ahead by the required # blocks */
                    if(lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE, SEEK_CUR) == -1) {
                        perror("Couldn't lseek to next header");
                        exit(errno);
                    }
                }
                continue;
            }
        }

        if(!verboseBool) {
            printf("%s\n", fullName); 
        }
        else {
            printf("Mode: %s, File: %s, size: %ld\n", perms, fullName, fileSize);
        }

        /* Skip over the body to next header */
        if(fileSize > 0) {
            /* If the file has size > 0, skip ahead by the required # blocks */
            if(lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE, SEEK_CUR) == -1) {
                perror("Couldn't lseek to next header");
                exit(errno);
            }
        }

        free(fullName);
        /* Clear for next read() */
        errno = 0;
    }
    close(fd);
    return 0;
}
