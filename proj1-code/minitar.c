#include "minitar.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 512
#define BUF_SIZE 512
/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
  // Have to initially set header's checksum to "all blanks"
  memset(header->chksum, ' ', 8);
  unsigned sum = 0;
  char *bytes = (char *)header;
  for (int i = 0; i < sizeof(tar_header); i++) {
    sum += bytes[i];
  }
  snprintf(header->chksum, 8, "%07o", sum);
}

/*
 * Populates a tar header block pointed to by 'header' with metadata about
 * the file identified by 'file_name'.
 * Returns 0 on success or -1 if an error occurs
 */
int fill_tar_header(tar_header *header, const char *file_name) {
  memset(header, 0, sizeof(tar_header));
  char err_msg[MAX_MSG_LEN];
  struct stat stat_buf;
  // stat is a system call to inspect file metadata
  if (stat(file_name, &stat_buf) != 0) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
    perror(err_msg);
    return -1;
  }

  strncpy(header->name, file_name,
          100);  // Name of the file, null-terminated string
  snprintf(header->mode, 8, "%07o",
           stat_buf.st_mode & 07777);  // Permissions for file, 0-padded octal

  snprintf(header->uid, 8, "%07o",
           stat_buf.st_uid);  // Owner ID of the file, 0-padded octal
  struct passwd *pwd =
      getpwuid(stat_buf.st_uid);  // Look up name corresponding to owner ID
  if (pwd == NULL) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s",
             file_name);
    perror(err_msg);
    return -1;
  }
  strncpy(header->uname, pwd->pw_name,
          32);  // Owner  name of the file, null-terminated string

  snprintf(header->gid, 8, "%07o",
           stat_buf.st_gid);  // Group ID of the file, 0-padded octal
  struct group *grp =
      getgrgid(stat_buf.st_gid);  // Look up name corresponding to group ID
  if (grp == NULL) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s",
             file_name);
    perror(err_msg);
    return -1;
  }
  strncpy(header->gname, grp->gr_name,
          32);  // Group name of the file, null-terminated string

  snprintf(header->size, 12, "%011o",
           (unsigned)stat_buf.st_size);  // File size, 0-padded octal
  snprintf(header->mtime, 12, "%011o",
           (unsigned)stat_buf.st_mtime);  // Modification time, 0-padded octal
  header->typeflag = REGTYPE;  // File type, always regular file in this project
  strncpy(header->magic, MAGIC, 6);  // Special, standardized sequence of bytes
  memcpy(header->version, "00", 2);  // A bit weird, sidesteps null termination
  snprintf(header->devmajor, 8, "%07o",
           major(stat_buf.st_dev));  // Major device number, 0-padded octal
  snprintf(header->devminor, 8, "%07o",
           minor(stat_buf.st_dev));  // Minor device number, 0-padded octal

  compute_checksum(header);
  return 0;
}

/*
 * Removes 'nbytes' bytes from the file identified by 'file_name'
 * Returns 0 upon success, -1 upon error
 * Note: This function uses lower-level I/O syscalls (not stdio), which we'll
 * learn about later
 */
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
  char err_msg[MAX_MSG_LEN];
  // Note: ftruncate does not work with O_APPEND
  int fd = open(file_name, O_WRONLY);
  if (fd == -1) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", file_name);
    perror(err_msg);
    return -1;
  }
  //  Seek to end of file - nbytes
  off_t current_pos = lseek(fd, -1 * nbytes, SEEK_END);
  if (current_pos == -1) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to seek in file %s", file_name);
    perror(err_msg);
    close(fd);
    return -1;
  }
  // Remove all contents of file past current position
  if (ftruncate(fd, current_pos) == -1) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
    perror(err_msg);
    close(fd);
    return -1;
  }
  if (close(fd) == -1) {
    snprintf(err_msg, MAX_MSG_LEN, "Failed to close file %s", file_name);
    perror(err_msg);
    return -1;
  }
  return 0;
}
// add_files: used in create_archive & append_files_to_archive for adding files
int add_files(const char *archive_name, const file_list_t *files, char *mode) {
  tar_header currHeader;  // keeps track of the current file header- changes
                          // within while loop
  char *currFileName;  // name of the current file
  node_t *currentNode =
      files->head;             // current node in the file_list_t data structure
  int validFile;               // was there an error when using fill_tar_header
  int bytes_read;              // records the number of bytes read in a file
  FILE *fc;                    // current file header
  char buffer[BUF_SIZE] = {};  // breaks the files into blocks
  FILE *fd = fopen(archive_name, mode);  // archive file header
  int closed = 0;

  if (fd == NULL) {  // error handling
    fprintf(stderr, "Failed to open file, errno: %d\n", errno);
    perror("Failed to open file");
    return -1;
  }
  if (strcmp(mode, "a") == 0) {  // mode used for append_files_to_archive
    if (remove_trailing_bytes(archive_name, 2 * BUF_SIZE) == -1) {
      return -1;
    }
  }
  while (currentNode != NULL) {
    currFileName = currentNode->name;
    fc = fopen(currFileName,
               "r");  // opens the current file in order to copy its contents
    if (fc == NULL) {
      fprintf(stderr, "Failed to open file, errno: %d\n", errno);
      perror("Failed to open file");
      return -1;
    }
    validFile = fill_tar_header(&currHeader, currFileName);  // error checking
    if (validFile == -1) {
      return -1;
    }
    fwrite(&currHeader, sizeof(currHeader), 1,
           fd);  // writes the header of the current file;
    bytes_read = fread(buffer, 1, BUF_SIZE, fc);

    while (bytes_read == BUF_SIZE) {  // allows for consistent block sizes until
                                      // there isn't enough information
      fwrite(buffer, 1, bytes_read, fd);
      bytes_read = fread(buffer, 1, BUF_SIZE, fc);
      // memset(buffer, 0, BUF_SIZE);
    }
    fwrite(buffer, 1, BUF_SIZE, fd);  // cleanup write
    closed = fclose(fc);              // close the current file
    if (closed != 0) {
      fprintf(stderr, "Failed to close file, errno: %d\n", errno);
      perror("Failed to close file");
      return -1;
    }
    currentNode = currentNode->next;  // move onto the next file
  }
  closed = fclose(fd);  // close the archive
  if (closed != 0) {
    fprintf(stderr, "Failed to close file, errno: %d\n", errno);
    perror("Failed to close file");
    return -1;
  }
  return 0;
}
int append_footer(const char *archive_name) {
  FILE *fd = fopen(archive_name, "a");
  int closed = 0;
  if (fd == NULL) {
    fprintf(stderr, "Failed to open file, errno: %d\n", errno);
    perror("Failed to open file");
    return -1;
  }
  char buffer[BUF_SIZE] = {};
  memset(buffer, 0, BUF_SIZE);
  fwrite(buffer, BUF_SIZE, 1, fd);
  fwrite(buffer, BUF_SIZE, 1, fd);
  closed = fclose(fd);  // close the archive
  if (closed != 0) {
    fprintf(stderr, "Failed to close file, errno: %d\n", errno);
    perror("Failed to close file");
    return -1;
  }
  return 0;
}
int create_archive(const char *archive_name, const file_list_t *files) {
  // TODO: Not yet implemented
  if (add_files(archive_name, files, "w") == -1) {
    return -1;
  }
  if (append_footer(archive_name) == -1) {
    return -1;
  }
  return 0;
}

int append_files_to_archive(const char *archive_name,
                            const file_list_t *files) {
  if (add_files(archive_name, files, "a") == -1) {
    return -1;
  }
  if (append_footer(archive_name) == -1) {
    return -1;
  }
  return 0;
}
int convert_files_size_to_blocks(int file_size) {
  int buff_blocks = file_size / BUF_SIZE;
  int remainder = file_size % BUF_SIZE;  //
  if (remainder != 0) {  // if the file size is not a multiple of BUF_SIZE, you
                         // need an additional block
    buff_blocks += 1;
  }
  return buff_blocks;
}
int get_archive_file_list(const char *archive_name, file_list_t *files) {
  char file_name[100] = {};
  char octal[12] = {};  // this is the file size as an octal string
  int file_blocks = 0;  // number of blocks the current file takes up
  int i = 0;
  char currHeader[512];
  unsigned file_size_int = 0;  // this is the file size as an int
  int closed = 0;
  int successful_seek = 0;
  int successful_scan = 0;
  FILE *fd = fopen(archive_name, "r");
  if (fd == NULL) {
    fprintf(stderr, "Failed to open file, errno: %d\n", errno);
    perror("Failed to open file");
    return -1;
  }
  fread(currHeader, 512, 1, fd);  // reads header block for file
  while (strlen(currHeader) != 0) {
    for (i = 0; i < 100; i++) {
      file_name[i] = currHeader[i];  // copies name
    }
    file_list_add(files, file_name);
    for (i = 124; i < 136; i++) {
      octal[i - 124] = currHeader[i];  // copies size as an octal string
    }
    successful_scan =
        sscanf(octal, "%011o",
               &file_size_int);  // converts from octal string to integer
    if (successful_scan == EOF) {
      fprintf(stderr, "Failed to obtain file size with scan, errno: %d\n",
              errno);
      perror("Failed to obtain file size");
      return -1;
    }
    file_blocks =
        convert_files_size_to_blocks(file_size_int);  // self explanitory
    successful_seek =
        fseek(fd, file_blocks * BUF_SIZE, SEEK_CUR);  // seek to next header
    if (successful_seek == -1) {
      fprintf(stderr, "Failed to seek in file, errno: %d\n", errno);
      perror("Failed to seek in file");
      return -1;
    }
    fread(currHeader, BUF_SIZE, 1, fd);  // fill current header
  }
  closed = fclose(fd);
  if (closed != 0) {
    fprintf(stderr, "Failed to close file, errno: %d\n", errno);
    perror("Failed to close file");
    return -1;
  }
  return 0;
}

int extract_files_from_archive(const char *archive_name) {
  char file_name[100] = {};  // file name read in from header block
  char octal[12] = {};       // file size stored as an octal string
  int file_blocks = 0;       // number of file blocks used for current file
  int i = 0;
  unsigned file_size_int = 0;  // size of file as an integer
  int offset = 0;        // used to calculate where to seek for the next header
  char currHeader[512];  // header of current file
  int closed = 0;
  int successful_seek = 0;
  int successful_scan = 0;
  FILE *fc;                             // current file descriptor
  FILE *fd = fopen(archive_name, "r");  // archive file descriptor
  if (fd == NULL) {
    fprintf(stderr, "Failed to open file, errno: %d\n", errno);
    perror("Failed to open file");
    return -1;
  }
  fread(currHeader, 512, 1, fd);  // fill current header
  while (strlen(currHeader) != 0) {
    for (i = 0; i < 100; i++) {
      file_name[i] = currHeader[i];  // retrieve file name
    }
    for (i = 124; i < 136; i++) {
      octal[i - 124] = currHeader[i];  // retrieve file size (stored as an
                                       // octal)
    }
    successful_scan =
        sscanf(octal, "%011o",
               &file_size_int);  // converts octal string to unsigned int
    if (successful_scan == EOF) {
      fprintf(stderr, "Failed to obtain file size with scan, errno: %d\n",
              errno);
      perror("Failed to obtain file size");
      return -1;
    }
    fc = fopen(file_name, "w");  // creates a new file
    if (fc == NULL) {
      fprintf(stderr, "Failed to open file, errno: %d\n", errno);
      perror("Failed to open file");
      return -1;
    }
    char buffer[file_size_int];            // file contents
    fread(buffer, file_size_int, 1, fd);   // store file contents
    fwrite(buffer, file_size_int, 1, fc);  // write contents to file
    closed = fclose(fc);                   // close new file
    if (closed != 0) {
      fprintf(stderr, "Failed to close file, errno: %d\n", errno);
      perror("Failed to close file");
      return -1;
    }
    file_blocks = convert_files_size_to_blocks(file_size_int);
    offset = (file_blocks * BUF_SIZE) -
             file_size_int;  // need to know where to start for the next read;
    successful_seek = fseek(fd, offset, SEEK_CUR);
    if (successful_seek == -1) {
      fprintf(stderr, "Failed to seek in file, errno: %d\n", errno);
      perror("Failed to seek in file");
      return -1;
    }
    fread(currHeader, 512, 1, fd);
  }
  closed = fclose(fd);
  if (closed != 0) {
    fprintf(stderr, "Failed to close file, errno: %d\n", errno);
    perror("Failed to close file");
    return -1;
  }
  return 0;
}
