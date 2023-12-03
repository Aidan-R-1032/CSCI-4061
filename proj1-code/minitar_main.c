#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"
int update_archive(const char* archive_name, file_list_t *files){
    file_list_t archive_files;
    file_list_init(&archive_files);
    int valid;
    valid = get_archive_file_list(archive_name, &archive_files);
    if(valid == -1){
        return -1;
    }
    valid = file_list_is_subset(files, &archive_files);
    if(valid == 0){
        file_list_clear(&archive_files);
        printf("Error: One or more of the specified files is not already present in archive\n");
        return -1;
    }
    valid = append_files_to_archive(archive_name, files);
    file_list_clear(&archive_files);
    return valid;
}
int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 0;
    }

    file_list_t files;
    file_list_init(&files);
    for(int i = 4; i < argc; i++){
        file_list_add(&files, argv[i]);
    }
    // TODO: Parse command-line arguments and invoke functions from 'minitar.h'
    // to execute archive operations
    if(strcmp(argv[2], "-f") != 0){
        printf("Indicate tar file with a preceding '-f'\n");
        return 0;
    }
    if(strcmp(argv[1], "-c") == 0){
        create_archive(argv[3], &files);
    }
    else if(strcmp(argv[1], "-a") == 0){
        append_files_to_archive(argv[3], &files);
    }
    else if(strcmp(argv[1], "-t") == 0){
        get_archive_file_list(argv[3], &files);
        node_t *currentFile = files.head;
        while(currentFile != NULL){
            printf("%s\n", currentFile->name);
            currentFile = currentFile->next;
        }
    }
    else if(strcmp(argv[1], "-u") == 0){
        update_archive(argv[3], &files);
    }
    else if(strcmp(argv[1], "-x") == 0){
        extract_files_from_archive(argv[3]);
    }
    file_list_clear(&files);
    return 0;
}
