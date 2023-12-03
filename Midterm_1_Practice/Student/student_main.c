#include <stdio.h>
#include <string.h>
#include "student.c"
int main(){
    student_t Aidan;
    strcpy(Aidan.name, "Aidan");
    Aidan.gpa = 3.95;
    Aidan.credits = 15;
    student_t Vaagish;
    strcpy(Vaagish.name, "Vaagish");
    Vaagish.gpa = 3.87;
    Vaagish.credits = 17;
    student_t Arthur;
    strcpy(Arthur.name, "Arthur");
    Arthur.gpa = 3.91;
    Arthur.credits = 19;
    student_list_t csci_2021;
    initialize_student_list(&csci_2021);
    add_student_to_list(Aidan, &csci_2021);
    add_student_to_list(Arthur, &csci_2021);
    add_student_to_list(Vaagish, &csci_2021);
    write_students(&csci_2021, "csci_2021.txt");
    get_highest_credits("csci_2021.txt", "highest_creds.txt");
    student_list_clear(&csci_2021);
    return 0;
}