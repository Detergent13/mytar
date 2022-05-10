#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){

    if (argc < 3){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(EXIT_FAILURE);
    }
    char *options = argv[1];
    int num_ops = len(options);

    if (num_ops < 2 || num_ops > 4){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(EXIT_FAILURE);
    }

}