#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){

    char *options;
    int num_ops, verbose, idx = 1;

    if (argc < 3){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    options = argv[1];
    num_ops = len(options);

    if (num_ops < 2 || num_ops > 4){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }

    if (options[0] != 'c' || options[0] != 't' || options[0] != 'x'){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        printf("second argument requires a c, t or x as first char\n");
        exit(EXIT_FAILURE);
    }

    while (options[idx] != 'f'){

        /* we hit the end of the second argument without encountering an 'f' */
        if (options[idx] == '\0'){
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            printf("f required in second argument\n")
            exit(EXIT_FAILURE);
        }
        else if (options[idx] == 'v'){
            verbose = 1;
        }
        else if(options[idx] == 'S'){
        /* add code to check for standards compliance */
        }

        else{
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            printf("Invalid character in second argument\n");
            exit(EXIT_FAILURE);
        }
        idx++;
    }




    return 0;
}