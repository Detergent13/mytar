#include "util.h"
#include "header.h"

#define MAX_NAME 100
#define MAX_PATH 256

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

void archive(char *path, int outfile){

    struct stat sb;
    struct header h;

    if (lstat(path, &sb) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    /* if it is a directory */
    if (S_ISDIR(sb.st_mode)){
        DIR *d;
        struct dirent *e;

        if(!(d = opendir(path))){
            perror("opendir");
            exit(EXIT_FAILURE);
        }

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

        strcpy(h -> uid, sb.st_uid);
        strcpy(h -> gid, sb.st_gid);
        h -> size = 0;
        strcpy(h -> mtime, sb.st_mtime);
        strcpy(h -> magic, "ustar");
        strcpy(h -> version, "00");
        set_uname(sb.st_uid, h -> uname);
        set_grname(sb.st_gid, h -> gname);

        /*mode,chksum,typeflag,linkname remaining */

        /* need to add writing to outfile */

        /*recursive aspect */
        while (e = readdir(d)){
            strcat(path, '/');
            strcat(path, e -> d_name);
            archive(path, outfile);
        }
        closedir(d);
        return;
    }

    /* if it is a regular file */
    if (S_ISREG(sb.st_mode)){

    }

}

int create_cmd(int verboseBool, int strictBool, char *root, int outfile) {

    archive(root, outfile);

    return 0;
}
