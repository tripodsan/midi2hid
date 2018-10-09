#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int verbose = 0;
    char *dhid = 0;
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-v] device\n", argv[0]);
                exit(-1);
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-v] device\n", argv[0]);
        exit(-1);
    }
    dhid = argv[optind];

    printf("verbose= %d, name=%s\n", verbose, dhid);
}