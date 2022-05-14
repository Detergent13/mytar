#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <grp.h>
#include <dirent.h>
#include <fcntl.h>


#include "util.h"
#include "header.h"
#include "given.h"

#define MAX_NAME 100
#define MAX_PATH 256
#define BLK_SIZE 512
#define LNK_SIZE 100
#define UID_SIZE 8
#define MTM_SIZE 12
#define NAME_SIZE 32

void set_uname(uid_t uid, char *[32]dest){
    struct passwd *pw;
    char *name = (char *)malloc(NAME_SIZE);

    if(!name){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (!(pw = getpwuid(uid))){
        perror("uid not found");
        exit(EXIT_FAILURE);
    }

    if (strlen(pw -> pw_name) >= NAME_SIZE){
        strncpy(name, pw -> pw_name, NAME_SIZE - 1);
    }

    else{
        strcpy(name, pw -> pw_name);
    }

    strcpy(dest, name);
    free(name);
    return;
}

void set_grname(gid_t gid, char *[32]dest){

    struct group *g;
    char *name = (char *)malloc(NAME_SIZE);

    if(!name){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (!(g = getgrgid(gid))){
        perror("gid not found");
        exit(EXIT_FAILURE);
    }

    if (strlen(g -> gr_name) >= NAME_SIZE){
        strncpy(name, g -> gr_name, NAME_SIZE - 1);
    }

    else{
        strcpy(name, g -> gr_name);
    }

    strcpy(dest, name);
    free(name);
    return;
}

/* returns the index that fits as much of the last part of "path"
 * into 100 chars */
int splice_name(char *path){

    int idx = (strlen(path) - 1) - MAX_NAME;

    while (path[idx] != '/'){
        if(!path[idx]){
            perror("path too long");
            return -1;
        }
    }

    return idx;

}

int write_header(char *path, int outfile, struct stat *sb, char typeflg, int strictBool){

    struct header h;

    /* to deal with padding 0s at the end */
    memset(&h, 0, BLK_SIZE);

    if (strlen(path) <= MAX_NAME){
        strcpy(h.name, path);
    }

    /* if the path name is longer than 100 chars */
    else{
        int splice_idx = splice_name(path);
        if (splice_idx == -1){
            return -1;
        }
        strcpy(h.name, path + splice_idx);
        strncpy(h.prefix, path, splice_idx);
    }

    if (sb -> st_uid > 07777777){
        if (strictBool){
            perror("Uid too large");
            return -1;
        }
        insert_special_int(h.uid, UID_SIZE, sb -> st_uid);
    }
    else{
        sprintf(h.uid, "%07o", (int) sb -> st_uid);
    }

    if (sb -> st_gid > 07777777){
        if (strictBool){
            perror("Gid too large");
            return -1;
        }
        insert_special_int(h.gid, UID_SIZE, sb -> st_gid);
    }

    else{
        sprintf(h.gid, "%07o", (int) sb -> st_gid);
    }

    /* check if its a file since dir and symlinks must be size 0 */
    if(S_ISREG(sb -> st_mode)){

        if (sb -> st_size > 077777777777){
            if (strictBool){
                perror("size too big");
                return -1;
            }
            insert_special_int(h.size, MTM_SIZE, sb -> st_size);
        }

        else{
            sprintf(h.size, "%011o", (int)sb -> st_size);
        }
    }

    else{
        sprintf(h.size, "%011o", 0);
    }

    *h.typeflag = typeflg;

    if (sb -> st_mtime > 077777777777){
        if (strictBool){
            perror("mtime too big");
            return -1;
        }
        insert_special_int(h.mtime, MTM_SIZE, sb -> st_mtime);
    }

    else{
        sprintf(h.mtime, "%011o", (int) sb -> st_mtime);
    }

    if (S_ISLNK(sb -> st_mode)){
        readlink(path, h.linkname, LNK_SIZE);
    }

    /* & with 07777 since the first part of the field is the filetype */
    sprintf(h.mode, "%07o", sb -> st_mode & 07777);

    strcpy(h.magic, "ustar");
    strcpy(h.version, "00");
    set_uname(sb -> st_uid, &h.uname);
    set_grname(sb -> st_gid, &h.gname);
    sprintf(h.chksum, "%08o", calc_checksum((char *)&h));

    if (write(outfile, &h, BLK_SIZE) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }

    return 0;

}

void write_content (int infile, int outfile){

    ssize_t num;
    char buff[BLK_SIZE];
    memset(buff, 0, BLK_SIZE);

    while((num = read(infile, buff, BLK_SIZE)) > 0){
        if (write(outfile, buff, BLK_SIZE) == -1){
            perror("write");
            exit(EXIT_FAILURE);
        }
        memset(buff, 0, BLK_SIZE);
    }

    return;

}

void archive(char *path, int outfile, int verboseBool, int strictBool){
    struct stat sb;
    if (verboseBool){
        printf("%s\n", path);
    }

    if (lstat(path, &sb) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    /* if it is a directory */
    if (S_ISDIR(sb.st_mode)){
        DIR *d;
        struct dirent *e;

        write_header(path, outfile, &sb, '5', strictBool);

        if(!(d = opendir(path))){
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        /*recursive aspect */
        while ((e = readdir(d))){
            if (strcmp(e -> d_name, ".") && strcmp(e -> d_name, "..")){

                /* "-1" is here since we havent taken into account the '/' */
                if ((strlen(path) + strlen(e -> d_name)) < MAX_PATH - 1){
                strcat(path, "/");
                strcat(path, e -> d_name);
                archive(path, outfile, verboseBool, strictBool);
                }

                else{
                    perror("path too long");
                }
            }
        }
        closedir(d);
        return;
    }

    /* if it is a regular file */
    else if (S_ISREG(sb.st_mode)){
        int infile;

        if ((infile = open(path, O_RDONLY)) == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }

        if((write_header(path, outfile, &sb, '0', strictBool)) != -1){
            write_content(infile, outfile);
        }
        close(infile);
        return;
    }

    else if (S_ISLNK(sb.st_mode)){
        write_header(path, outfile, &sb, '2', strictBool);
        return;
    }

    return;

}

int create_cmd(int verboseBool, int strictBool, char *start, int outfile) {

    char *path = (char *) malloc(MAX_PATH);
    char *stop_blocks = (char *)calloc(2, BLK_SIZE);

    if (!path || !stop_blocks){
        perror("malloc (or calloc)");
        exit(EXIT_FAILURE);
    }

    strcpy(path, start);

    archive(path, outfile, verboseBool, strictBool);

    if (write(outfile, stop_blocks, BLK_SIZE * 2) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }

    return 0;
}
