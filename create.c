#include "util.h"
#include "header.h"

#define MAX_NAME 100
#define MAX_PATH 256
#define BLK_SIZE 512
#define LNK_SIZE 100

void set_uname(uid_t uid, char *dest){
    struct passwd *pw;

    if (!(pw = getpwuid(uid))){
        perror("uid not found");
        exit(EXIT_FAILURE);
    }

    strcpy(dest, pw -> pw_name);
    return;
}

void set_grname(gid_t gid, char *dest){

    struct group *g;

    if (!(g = getgrgid(gid))){
        perror("gid not found");
        exit(EXIT_FAILURE);
    }

    strcpy(dest, g -> gr_name);
    return;
}

/* returns the index that fits as much of the last part of "path"
 * into 100 chars */
int splice_name(char *path){

    int idx = (len(path) - 1) - MAX_NAME;

    while (path[idx++] != '/'){
        /* empty body */
    }

    return idx;

}

void write_header(char *path, int outfile, struct stat *sb, char typeflg){

    struct header h;

    /* to deal with padding 0s at the end */
    memset(h, 0, BLK_SIZE);

    if (len(path) <= MAX_NAME){
        strcpy(h -> name, path);
        strcpy(h -> prefix, '\0');
    }

    /* if the path name is longer than 100 chars */
    else{
        int splice_idx = splice_name(path);
        strcpy(h -> name, path + splice_idx);
        strncpy(h -> prefix, path, splice_idx);
    }

    strcpy(h.uid, sb -> st_uid);
    strcpy(h.gid, sb -> st_gid);

    /* check if its a file since dir and symlinks must be size 0 */
    if(S_ISREG(sb -> st_mode)){
        sprintf(h.size, "%011o", (int)sb -> st_size);
    }

    else{
        sprintf(h.size, "%011o", 0);
    }

    *h.typeflag = typeflag;
    if (S_ISLINK(sb -> st_mode)){
        readlink(path, h.linkname, LNK_SIZE);
    }

    sprintf(h.mtime, "%011o", (int) sb -> st_mtime);
    strcpy(h.magic, "ustar");
    strcpy(h.version, "00");
    set_uname(sb.st_uid, &h.uname);
    set_grname(sb.st_gid, &h.gname);

    /*chksum remaining */


    if (write(outfile, h, BLK_SIZE) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }

    return;

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

void archive(char *path, int outfile, int verboseBool){

    if (verboseBool){
        printf("%s\n", path);
    }

    struct stat sb;

    if (lstat(path, &sb) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    /* if it is a directory */
    if (S_ISDIR(sb.st_mode)){
        DIR *d;
        struct dirent *e;

        write_header(path, outfile, &sb, '5');

        if(!(d = opendir(path))){
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        /*recursive aspect */
        while (e = readdir(d)){
            if (strcmp(e -> d_name, '.') && strcmp(e -> d_name, '..')){
                strcat(path, '/');
                strcat(path, e -> d_name);
                archive(path, outfile, verboseBool);
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

        write_header(path, outfile, &sb, '0');
        write_content(infile, outfile);
        return;
    }

    else if (S_ISLNK(sb.st_mode)){

    write_header(path, outfile, &sb, '2');

    return;

    }

    return;

}

int create_cmd(int verboseBool, int strictBool, char *root, int outfile) {

    archive(root, outfile, verboseBool);

    return 0;
}
