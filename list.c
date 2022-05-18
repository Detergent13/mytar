#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h> 
#include "util.h"
#include "header.h"
#include "given.h"

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
#define MTIME_STR_LEN 16

extern int errno;

int list_cmd(char* fileName, char *directories[], int numDirectories,
    int verboseBool, int strictBool) {

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

    errno = 0;
    while(read(fd, &headerBuffer, sizeof(struct header)) > 0) {
        char *fullName;
        int i, expectedChecksum, readChecksum;
        unsigned long int fileSize;
        char *ownerGroup;
        char perms[] = "-rwxrwxrwx";
        int mask = STARTING_MASK;
        int readMode = strtol(headerBuffer.mode, NULL, OCTAL);
        struct tm m_time;
        long int readTime;
        char *mtime_str;

        /* Check read() error */
        if(errno) {
            perror("Couldn't read header");
            exit(errno);
        }

        fileSize = strtol(headerBuffer.size, NULL, OCTAL);
        expectedChecksum = calc_checksum((unsigned char *)&headerBuffer);
        readChecksum = strtol(headerBuffer.chksum, NULL, OCTAL);
   
        /* Check for valid end of archive */
        if(readChecksum == 0 && expectedChecksum == EMPTY_BLOCK_CHKSUM) {
            int nextExpected;
            int nextRead;

            /* Read next block */
            errno = 0;
            read(fd, &headerBuffer, sizeof(struct header));
            if(errno) {
                perror("Couldn't check next block");
                exit(errno);
            }

            nextExpected = calc_checksum((unsigned char *)&headerBuffer);
            nextRead = strtol(headerBuffer.chksum, NULL, OCTAL);

            /* Check if next block is also all zeroes */
            if(nextRead != 0 || nextExpected != EMPTY_BLOCK_CHKSUM) {
                fprintf(stderr, "Archive is corrupted! Exiting.");
                exit(EXIT_FAILURE);
            }

            /* Test if there's any unexpected data at the end. */
            if(read(fd, &headerBuffer, 1) != 0) {
                fprintf(stderr, "Archive is corrupted! Exiting.");
                exit(EXIT_FAILURE);
            }

            /* If we haven't errored out by now, we must be at the end
             * of a valid archive! We're all done. */
            exit(EXIT_SUCCESS);
        }
    
        /* Validate checksum */
        if(expectedChecksum != readChecksum) {
            fprintf(stderr,
                    "Expected checksum %d doesn't match read checksum %d\n",
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
        if(strictBool && strncmp("00", headerBuffer.version, VERSION_LEN) != 0){
            /* The .2 limits the # chars we print */
            fprintf(stderr, "Version doesn't check out: \"%.2s\"\n",
                (char *)&headerBuffer.version);
            exit(EXIT_FAILURE);
        }       

        /* Add d or l for directory/link */
        if(*(headerBuffer.typeflag) == DIR_FLAG) {
            *perms = 'd';
        }
        else if(*(headerBuffer.typeflag) == SYM_FLAG) {
            *perms = 'l';
        }

        /* Check each perms bit and set accordingly */
        for(i = 0; i < PERMS_LEN; i++) {
            if(!(mask & readMode)) {
                perms[i + 1] = '-';
            }
            mask >>= 1;
        }

        errno = 0;
        ownerGroup = calloc(OWNER_LEN + 1, sizeof(char));
        if(errno) {
            perror("Couldn't calloc ownerGroup");
            exit(errno);
        }

        if(headerBuffer.uname[0]) {
            snprintf(ownerGroup, OWNER_LEN + 1, "%s/%s",
                (char *)&headerBuffer.uname, 
                (char*)&headerBuffer.gname);
        }
        else {
            int uidCheck = strtol(headerBuffer.uid, NULL, OCTAL);
            if(uidCheck & SPECIAL_INT_MASK) {
                /* extract special ints here */
                uint32_t special_uid = 
                    extract_special_int(headerBuffer.uid,
                    sizeof(headerBuffer.uid));
                uint32_t special_gid =
                     extract_special_int(headerBuffer.gid,
                     sizeof(headerBuffer.gid));
                snprintf(ownerGroup, OWNER_LEN + 1, "%ld/%ld",
                     (long int)special_uid,
                     (long int)special_gid);
            }
            else {
                /* strtol as usual and concat */
                long int uid = strtol(headerBuffer.uid, NULL, OCTAL);
                long int gid = strtol(headerBuffer.gid, NULL, OCTAL);
                snprintf(ownerGroup, OWNER_LEN + 1, "%ld/%ld", uid, gid); 
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
            strncpy(fullName, (char *)&headerBuffer.prefix,
                 sizeof(headerBuffer.prefix));
            strcat(fullName, "/");
            strncat(fullName, (char *)&headerBuffer.name,
                 sizeof(headerBuffer.name));
        }
        else {
            strncpy(fullName, (char *)&headerBuffer.name,
                sizeof(headerBuffer.name));
        }

        errno = 0;
        mtime_str = calloc(MTIME_STR_LEN + 1, sizeof(char));
        if(errno) {
            perror("Couldn't calloc mtime_str");
            exit(errno);
        }

        /* Read mtime and format it into a tm struct */
        readTime = strtol(headerBuffer.mtime, NULL, OCTAL);
        memcpy(&m_time, localtime(&readTime), sizeof(struct tm));

        /* Format the time into a string */
        errno = 0;
        strftime((char *)mtime_str, MTIME_STR_LEN + 1, "%Y-%m-%d %H:%M",
             &m_time);
        if(errno) {
            perror("Couldn't format time");
            exit(errno);
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
                    /* Skip the body */
                    if(lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE,
                             SEEK_CUR) == -1) {
                        perror("Couldn't lseek to next header");
                        exit(errno);
                    }
                }
                continue;
                free(fullName);
            }
        }

        if(!verboseBool) {
            printf("%s\n", fullName); 
        }
        else {
            printf("%10.10s %17.17s %8ld %16.16s %s\n",
                 perms, ownerGroup, fileSize, mtime_str, fullName);
        }

        /* Skip over the body to next header */
        if(fileSize > 0) {
            /* If the file has size > 0, skip ahead by the required # blocks */
            if(lseek(fd, (fileSize / BLOCKSIZE + 1) * BLOCKSIZE, SEEK_CUR)
                == -1) {
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
