#include <stdio.h>

#define BUF_SIZE 4096

/*
 * Copy the contents of one file into another file
 *   source_file: Name of the source file to copy from
 *   dest_file: Name of the destination file to copy to
 * The destination file is overwritten if it already exists
 * Returns 0 on success and -1 on error
 */
int copy_file(const char *source_file, const char *dest_file) {
    // TODO Not yet implemented
    FILE* sf = fopen(source_file, "r");
    if (sf == NULL){
        return -1;
    }
    FILE* df = fopen(dest_file, "w");
    if (df == NULL){
        return -1;
    }
    int buf[BUF_SIZE];
    int bytes_read = fread(buf, 1, BUF_SIZE, sf);
    while(bytes_read == BUF_SIZE){
        fwrite(buf, bytes_read, 1, df);
        bytes_read = fread(buf, 1, BUF_SIZE, df);
    }
    fwrite(buf, bytes_read, 1, df);
    fclose(sf);
    fclose(df);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <source> <dest>\n", argv[0]);
        return 1;
    }

    // copy_file already prints out any errors
    if (copy_file(argv[1], argv[2]) != 0) {
        return 1;
    }
    return 0;
}
