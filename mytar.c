#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){

    if (argc < 3 || argc > 4){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(EXIT_FAILURE);
    }
    char *options = argv[2];

}