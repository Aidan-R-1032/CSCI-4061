#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "http.h"

#define BUFSIZE 4096

const char *get_mime_type(const char *resource_name_extension) {
    if (strcmp(".txt", resource_name_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", resource_name_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", resource_name_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", resource_name_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", resource_name_extension) == 0) {
        return "application/pdf";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    // copy file descriptor so we can read from it without modifying its offset
    int fd_copy = dup(fd);
    if (fd_copy == -1) {
        perror("dup");
        return -1;
    }
    FILE *socket_stream = fdopen(fd_copy, "r");
    if (socket_stream == NULL) {
        perror("fdopen");
        close(fd_copy);
        return -1;
    }
    // Disable the usual stdio buffering
    if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        fclose(socket_stream);
        return -1;
    }

    // stdio FILE * gives us 'fgets()' to easily read line by line - resource name should always be in first line
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    if (fgets(buf, BUFSIZE, socket_stream) == NULL) { //is empty request
        return -1;
    }    
    
    int iterator_limit = strlen(buf);
    for(int i = 4; i < iterator_limit; i++){ // resource name begins at index 4 as it's preceded by 'G', 'E', 'T', and ' '
        if(buf[i] == ' '){
            resource_name[i - 4] = '\0'; // add null terminator at very end
            break;
        }
        resource_name[i - 4] = buf[i];
    }

    if (fclose(socket_stream) != 0) { // error check closing the socket_stream
        perror("fclose");
        return -1;
    }
    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    struct stat path_info;
    char header[BUFSIZE];
    memset(header, 0, BUFSIZE);
    
    strcpy(header, "HTTP/1.0 ");    // beginning of a proper HTTP response
    if(stat(resource_path, &path_info) == -1){  // determine if the resource actually exists
        strcat(header, "404 Not Found\r\nContent-Length: 0\r\n\r\n");   // invalid request
        if(write(fd, header, strlen(header)) == -1){    // error check writing header
            perror("write");
            close(fd);
            return -1;
        }
    }
    else{
        strcat(header, "200 OK\r\n"); // valid request
        char content_type[BUFSIZE];
        memset(content_type, 0, BUFSIZE);

        // need to find the extention in order to get the content_types
        int cut_idx = -1;
        int length = strlen(resource_path);
        int remaining = 0;
        for(int i = 0; i < length; i++){
            if(resource_path[i] == '.'){
                cut_idx = i;
                remaining = length - i;
                break;
            }
        }
        if(cut_idx == -1){
            printf("Invalid file type\n");
            return -1;
        }

        char extension[BUFSIZE];
        memset(extension, 0, BUFSIZE);
        for(int j = 0; j < remaining; j++){
            extension[j] = resource_path[j + cut_idx];
        }

        snprintf(content_type, BUFSIZE, "Content-Type: %s\r\n", get_mime_type(extension)); // append extention string to content_type
        strcat(header, content_type);
        
        char content_len[BUFSIZE];
        memset(content_len, 0, BUFSIZE);
        snprintf(content_len, BUFSIZE, "Content-Length: %ld\r\n\r\n", path_info.st_size);
        strcat(header, content_len);
       
        if(write(fd, header, strlen(header)) == -1){ // error check writing header
            perror("write");
            close(fd);
            return -1;
        }
        int rp_fd = open(resource_path, O_RDONLY);
        if (rp_fd < 0){ // error checking open
            perror("open");
            return -1;
        }
        int bytes_read = 0;
        int bytes_wrote = 0;
        long bytes_wrote_total = 0;
        
        char buf[BUFSIZE];
        memset(buf, 0, BUFSIZE);
        
        // writing file contents to client
        while(bytes_wrote_total < path_info.st_size){
            memset(buf, 0, BUFSIZE);
            bytes_read = read(rp_fd, buf, BUFSIZE);
            if(bytes_read < 0){
                close(rp_fd);
                printf("Read Failed\n");
                return -1;
            }

            bytes_wrote = write(fd, buf, BUFSIZE);
            if(bytes_wrote < 0){
                close(rp_fd);
                printf("Write failed\n");
                return -1;
            }
            bytes_wrote_total += bytes_wrote;
        }
        close(rp_fd); // closing resource 
    }
    return 0;
}
