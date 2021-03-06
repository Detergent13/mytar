#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>
#include "util.h"
#include "header.h"

#define OCTAL 8
#define BLK_SIZE 512
#define REG_FLAG '0'
#define REG_FLAG_ALT '\0'
#define SYM_FLAG '2'
#define DIR_FLAG '5'
#define MAX_NAME 100
#define MAX_PREFIX 155
#define MAX_LINK 100
#define EMPTY_BLOCK_CHKSUM 256
#define MAGIC_LEN 6
#define VERSION_LEN 2

/* checks if the path contains non-existent dirs and creates them */
void check_dirs(char *path){

    int idx;
    char *cpy;
    mode_t perms = S_IRWXU | S_IRWXG | S_IROTH;
    errno = 0;
    cpy = (char *)malloc((strlen(path) + 1) * sizeof(char));

    if (errno){
        perror("malloc failed in check_dirs");
        exit(EXIT_FAILURE);
    }

    strcpy(cpy, path);

    for (idx = 0; cpy[idx]; idx++){
        if (cpy[idx] == '/'){
            if (idx >= strlen(path) - 1){
                free(cpy);
                return;
            }
            cpy[idx] = '\0';
            if(mkdir(cpy, perms) && errno != EEXIST) {
                perror("Couldn't mkdir");
                exit(errno);
            }
            cpy[idx] = '/';
        }
    }

    free(cpy);
    return;

}

void extract_file_content (int infile, int outfile, unsigned int file_size){
    char *buff;
    int padding = file_size % BLK_SIZE;

    errno = 0;
    buff = malloc(sizeof(char) * file_size);
    if(errno) {
        perror("Couldn't malloc buff");
        exit(errno);
    }

    if(read(infile, buff, file_size) == -1) {
        perror("Couldn't read from archive");
        exit(errno);
    }

    if(write(outfile, buff, file_size) == -1) {
        perror("Couldn't write file");
        exit(errno);
    }

    if(lseek(infile, BLK_SIZE - padding, SEEK_CUR) == -1){
        perror("lseek padding of file content failed");
        exit(errno);
    }
    
    return;
}

int extract_cmd(char* fileName, char *directories[], int numDirectories,
         int verboseBool, int strictBool) {
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
        unsigned char typeFlag = headerBuffer.typeflag[0];
        struct stat statBuffer;
        struct utimbuf newTime;
        char *filePath;
        char *pathNoLead;
        mode_t permissions, default_perms;
        int expectedChecksum, readChecksum;
        struct timespec times[2];

        /* Check read() error */
        if(errno) {
            perror("Couldn't read header");
            exit(errno);
        }


        fileSize = strtol(headerBuffer.size, NULL, OCTAL);
        expectedChecksum = calc_checksum((unsigned char *) &headerBuffer);
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

        errno = 0;
        /* +4 for leading "./", concat "/", and null-terminator */
        filePath = calloc(MAX_NAME + MAX_PREFIX + 4, sizeof(char));
        if(errno) {
            perror("Couldn't calloc filePath");
            exit(errno);
        }

        /* NOTE: We're using strncat to avoid undefined behavior.
         * Prefix and name aren't necessarily null-terminated, and the normal
         * strcat just looks for a null terminator in the source string
         * to stop. strncat stops at the null terminator or after n chars,
         * whichever comes first. */

        /* Leading ./ for a valid relative path */
        strcat(filePath, "./");
        /* If there's something in prefix, add it and a slash */
        if(headerBuffer.prefix[0]) {
            strncat(filePath, (char *)&headerBuffer.prefix, MAX_PREFIX);
            strcat(filePath, "/");
        }
        /* Add the name unconditionally */
        strncat(filePath, (char *)&headerBuffer.name, MAX_NAME);

        /* Make a path without the leading ./ for directory validation */
        pathNoLead = filePath + 2;
        /* If we've passed a non-null directories[], check all directories
         * against the beginning of current fullName and don't print if
         * it doesn't match an element of directories[]. */
        if(directories) {
            int i;
            int inDirectoriesBool = 0;
            for(i = 0; i < numDirectories; i++) {
                int pathLength = strlen(directories[i]);
                if(strncmp(pathNoLead, directories[i], pathLength) == 0) {
                    inDirectoriesBool = 1;
                    break;
                }
            }
            if(!inDirectoriesBool) {
                if(fileSize > 0) {
                    /* Skip the body */
                    if(lseek(fd, (fileSize / BLK_SIZE + 1) * BLK_SIZE,
                             SEEK_CUR) == -1) {
                        perror("Couldn't lseek to next header");
                        exit(errno);
                    }
                }
                continue;
                free(filePath);
            }
        }

        if(verboseBool) {
            printf("%s", filePath);
        }

        check_dirs(pathNoLead);

        /* we dont need a second arg since we are guaranteed a string of
         * octal digits */
        permissions = (mode_t)strtol(headerBuffer.mode, NULL, OCTAL);

        default_perms = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

        if (permissions & S_IXUSR || permissions & S_IXGRP ||
            permissions & S_IXOTH){
            default_perms = default_perms|S_IRWXU|S_IRWXG|S_IRWXO;
        }

        permissions |= default_perms;

        /* P.S. I know that I don't have to put every case in curly brackets
         * normally. But this gets around a quirk of C that throws a fit
         * about mixed declarations otherwise. Something about a case just
         * being a label, not a separate scope. The brackets make it a 
         * separate scope. Thanks StackOverflow! Cool tidbit. */
        switch (typeFlag) {
            case REG_FLAG_ALT:
            case REG_FLAG: {
                int new_file;
                /* NOTE: This currently grants everything to user.
                 * We'll either need to change these perms in this call,
                 * or just set them again later. If user mysteriously has
                 * perms, this is probably the culprit. */

                /* old ver
                if((new_file = creat(filePath, permissions)) == -1) {
                    perror("Couldn't create file");
                    exit(errno);
                }
                */

                /* using open */

                if((new_file = open(filePath, O_RDWR|O_CREAT|O_TRUNC,
                                    permissions)) == -1){
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                extract_file_content(fd, new_file, fileSize);
                break;
            }
            case SYM_FLAG: {
                /* We're doing this to catch issues with it possibly not being
                 * null-terminated. Basically just slapping on our own
                 * null-terminator(s) at the end. */ 
                char *linkValue; 
                errno = 0;
                linkValue = calloc(MAX_LINK + 1, sizeof(char));
                if(errno) {
                    perror("Couldn't calloc linkValue");
                    exit(errno);
                }

                strncpy(linkValue, (char *)&headerBuffer.linkname,
                    MAX_LINK);

                /* Make a symlink with name filePath that points
                 * to linkValue. It's easy to get mixed up here! */
                errno = 0;
                if(symlink(linkValue, filePath) && errno != EEXIST) {
                    perror("Couldn't create symlink");
                    exit(errno);
                }

                free(linkValue);
                break;
            }
            case DIR_FLAG: {
                /* NOTE: Again, watch out for these perms. */
                errno = 0;
                if(mkdir(filePath, permissions) && errno != EEXIST) {
                    perror("Couldn't mkdir");
                    exit(errno);
                }
                break;
            }
            default: {
                fprintf(stderr, "Invalid typeflag! '%c'", typeFlag);
                exit(EXIT_FAILURE);
            }
        }
       
        
        /* Stat the created file to get its current times */
        if(lstat(filePath, &statBuffer)) {
            perror("Failed to stat created file!");
            exit(errno);
        }

        /* Preserve atime */
        newTime.actime = statBuffer.st_atime;
        /* Set mtime to mtime from header */
        newTime.modtime = strtol(headerBuffer.mtime, NULL, OCTAL);

        times[0].tv_sec = newTime.actime;
        times[0].tv_nsec = 0;
        times[1].tv_sec = newTime.modtime;
        times[1].tv_nsec = 0;

        if (utimensat(AT_FDCWD, filePath, times, AT_SYMLINK_NOFOLLOW)){
            perror("Couldn't set utime");
            exit(errno);
        }

        /* old ver
        if(utime(filePath, &newTime)) {
            perror("Couldn't set utime");
            exit(errno);
        }
        */

        /* NOTE: We shouldn't be touching the created file after this.
         * The mtime could be disturbed. Do any operations on it before 
         * setting mtime. */

        free(filePath);
        errno = 0;
    }
    close(fd);
 
    /* Set directory mtimes on second pass */ 
    errno = 0;
    while(read(fd, &headerBuffer, sizeof(struct header)) > 0) {
        unsigned long int fileSize;
        char *filePath, *pathNoLead;
        fileSize = strtol(headerBuffer.size, NULL, OCTAL);
      
        /* Check read() error */
        if(errno) {
            perror("Couldn't read header");
            exit(errno);
        }

        filePath = calloc(MAX_NAME + MAX_PREFIX + 4, sizeof(char));
        if(errno) {
            perror("Couldn't calloc filePath");
            exit(errno);
        }

        strcat(filePath, "./");
        /* If there's something in prefix, add it and a slash */
        if(headerBuffer.prefix[0]) {
            strncat(filePath, (char *)&headerBuffer.prefix, MAX_PREFIX);
            strcat(filePath, "/");
        }
        /* Add the name unconditionally */
        strncat(filePath, (char *)&headerBuffer.name, MAX_NAME);

        pathNoLead = filePath + 2;

        if(directories) {
            int i;
            int inDirectoriesBool = 0;
            for(i = 0; i < numDirectories; i++) {
                int pathLength = strlen(directories[i]);
                if(strncmp(pathNoLead, directories[i], pathLength) == 0){
                    inDirectoriesBool = 1;
                    break;
                }
            }
            if(!inDirectoriesBool) {
                if(fileSize > 0) {
                    /* Skip the body */
                    if(lseek(fd, (fileSize / BLK_SIZE + 1) * BLK_SIZE,
                             SEEK_CUR) == -1) {
                        perror("Couldn't lseek to next header");
                        exit(errno);
                    }
                }
                continue;
                free(filePath);
            }
        }

        
        if(*headerBuffer.typeflag == DIR_FLAG) {
            struct stat statBuffer;
            struct utimbuf newTime;
            char *filePath;

            errno = 0;
            /* +4 for leading "./", concat "/", and null-terminator */
            filePath = calloc(MAX_NAME + MAX_PREFIX + 4, sizeof(char));
            if(errno) {
                perror("Couldn't calloc filePath");
                exit(errno);
            }

            /* Leading ./ for a valid relative path */
            strcat(filePath, "./");
            /* If there's something in prefix, add it and a slash */
            if(headerBuffer.prefix[0]) {
                strncat(filePath, (char *)&headerBuffer.prefix, MAX_PREFIX);
                strcat(filePath, "/");
            }
            /* Add the name unconditionally */
            strncat(filePath, (char *)&headerBuffer.name, MAX_NAME);

        
            if(lstat(filePath, &statBuffer)) {
                perror("Failed to stat created file!");
                exit(errno);
            } 
 
            /* Preserve atime */
            newTime.actime = statBuffer.st_atime;
            /* Set mtime to mtime from header */
            newTime.modtime = strtol(headerBuffer.mtime, NULL, OCTAL);
            /* Actually write the time */
            if(utime(filePath, &newTime)) {
                perror("Couldn't set utime");
                exit(errno);
            }
        }

        /* Skip over the body to next header */
        if(fileSize > 0) {
            /* If the file has size > 0, skip ahead by the required # blocks */
            if(lseek(fd, (fileSize / BLK_SIZE + 1) * BLK_SIZE, SEEK_CUR) == -1){
                perror("Couldn't lseek to next header");
                exit(errno);
            }
        }
        errno = 0;
        free(filePath);
    }
    return 0;
}
