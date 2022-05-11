#include "util.h"
#include "header.h"

void set_uname(uid_t uid, char *dest){
    struct passwd *pw;

    if (!(pw = getpwuid(uid))){
        perror("uid not found");
        exit(EXIT_FAILURE);
    }

    dest = pw -> pw_name;
    return;
}

void set_grname(gid_t gid, char *dest){

    struct group *g;

    if (!(g = getgrgid(gid))){
        perror("gid not found");
        exit(EXIT_FAILURE);
    }

    dest = g -> gr_name;
    return;
}

void archive(char *path, int outfile){

    struct stat sb;
    struct header h;

    if (stat(path, &sb) == -1){
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

        h -> name = path;
        h -> uid = sb.st_uid;
        h -> gid = sb.st_gid;
        h -> size = 0;
        h -> mtime = sb.st_mtime;
        h -> magic = "ustar";
        h -> version = "00";
        set_uname(sb.st_uid, h -> uname);
        set_grname(sb.st_gid, h -> gname);

        /*mode,chksum,typeflag,linkname,prefix remaining */

        /* need to add writing to outfile */

        /*recursive aspect */
        while (e = readdir(d)){
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
