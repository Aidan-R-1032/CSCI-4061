#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int total_dir_size() {
    // Open the current working directory, relative path "."
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Failed to open current directory");
        return -1;
    }

    struct dirent *current_ent;
    int total_size = 0;
    // Read through all directory entries
    while ((current_ent = readdir(dir)) != NULL) {
        // Check that the current entry is for a regular file
        // We'll ignore subdirectories and symbolic links
        if (current_ent->d_type == DT_REG) {
            // Read in file metadata from inode
            struct stat stat_buf;
            if (stat(current_ent->d_name, &stat_buf) == -1) {
                perror("Failed to stat file");
                closedir(dir);
                return -1;
            }
            total_size += stat_buf.st_size;
        }
    }

    // Close directory
    closedir(dir);
    return total_size;
}

int main() {
    int dir_size = total_dir_size();
    if (dir_size == -1) {
        return 1;
    }

    printf("Total size: %u\n", dir_size);
    return 0;
}
