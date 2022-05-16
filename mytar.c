#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "mytar.h"

extern int errno;

int main(int argc, char *argv[]){

    char *options;
    int num_ops, verboseBool = 0, strictBool = 0, idx = 1, path_idx = 3;
    char **paths;

    if (argc < 3){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    options = argv[1];
    num_ops = strlen(options);

    if (num_ops < 2 || num_ops > 4){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }

    if (options[0] != 'c' && options[0] != 't' && options[0] != 'x'){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        printf("second argument requires a c, t or x as first char\n");
        exit(EXIT_FAILURE);
    }

    while (options[idx] != 'f'){

        /* we hit the end of the second argument without encountering an 'f' */
        if (options[idx] == '\0'){
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            printf("f required in second argument\n");
            exit(EXIT_FAILURE);
        }
        else if (options[idx] == 'v'){
            verboseBool = 1;
        }
        else if(options[idx] == 'S'){
            strictBool = 1;
        }

        else{
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            printf("Invalid character in second argument\n");
            exit(EXIT_FAILURE);
        }
        idx++;
    }

    errno = 0;
    paths = malloc(sizeof(char *) * (argc - path_idx));
    if(errno) {
        perror("Couldn't malloc paths");
        exit(errno);
    }

    idx = 0;
    while(path_idx < argc){
        paths[idx++] = argv[path_idx++];
    }

    switch(options[0]){
        case 'c':
            create_cmd(verboseBool, strictBool, idx, argv[2], paths);
            break;

        case 't':
            if(idx) {
                list_cmd(argv[2], paths, idx, verboseBool, strictBool);
            }
            else {
                list_cmd(argv[2], NULL, 0, verboseBool, strictBool);
            }
            break;

        case 'x':
            extract_cmd(argv[2], verboseBool, strictBool);
            break;
    }

    return 0;
}
