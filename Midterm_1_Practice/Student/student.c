#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct{
    char name[32];
    float gpa;
    unsigned int credits;
} student_t;

typedef struct node{
    student_t student_info;
    struct node *next;
} node_t;

typedef struct{
    node_t *head;
    int size;
} student_list_t;

int initialize_student_list(student_list_t *students){
    students->size = 0;
    students->head = NULL;
    return 0;
}

int add_student_to_list(student_t student, student_list_t *students){
    if(students->head == NULL){
        students->head = malloc(sizeof(node_t));
        students->head->student_info = student;
        students->head->next = NULL;
        students->size = 1;
        return 0;
    }
    node_t* ptr = students->head;
    while(ptr->next != NULL){
        ptr = ptr->next;
    }
    ptr->next = malloc(sizeof(node_t));
    ptr->next->student_info = student;
    ptr->next->next = NULL;
    students->size++;
    return 0;
}

void student_list_clear(student_list_t *list) {
    node_t *current = list->head;
    while (current != NULL) {
        node_t *to_free = current;
        current = current->next;
        free(to_free);
    }
    list->head = NULL;
    list->size = 0;
}

int write_students(student_list_t *students, char* file_name){
    FILE* fd = fopen(file_name, "w");
    node_t *student = students->head;
    char *buffer[100];
    memset(buffer, 0, 100);
    if(fd == NULL){
        printf("Could not open file, errno: %d\n", errno);
        perror("Could not open file");
        return -1;
    }
    while(student != NULL){
        fwrite(&(student->student_info), 1, sizeof(student_t), fd);
        fwrite(buffer, 100, 1, fd);
        student = student->next;
    }
    fclose(fd);
    return 0;
}

int get_highest_credits(char *in_file, char *out_file){
    student_t highestStudent;
    highestStudent.credits = 0;
    student_t curStudent;
    FILE* fd = fopen(in_file, "r");
    if(fd == NULL){
        printf("Could not open file, errno: %d\n", errno);
        perror("Could not open file");
        return -1;
    }
    while(fread(&curStudent, sizeof(student_t), 1, fd) != 0){//read until EOF or Error has occurred
        if(curStudent.credits > highestStudent.credits){//able to access struct fields after read
            highestStudent = curStudent;
        }
        fseek(fd, 100, SEEK_CUR);
    }
    fclose(fd);
    FILE *fc = fopen(out_file, "w");
    if(fc == NULL){
        printf("Could not open file, errno: %d\n", errno);
        perror("Could not open file");
        return -1;
    }
    fwrite(&highestStudent, sizeof(student_t), 1, fc);
    fclose(fd);
    return 0;
}