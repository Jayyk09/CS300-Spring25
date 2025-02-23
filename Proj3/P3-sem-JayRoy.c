/*
 * P3-sem-YourFirstLastName.c
 * Full Name: Jay Roy
 * CWID: 12342760
 * CS 300, Spring 2025 – Interview Booth Project
 *
 * Synchronization is implemented using mutexes and semaphores.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <unistd.h>
#include "mytime.h"  // Assumes mytime(left, right) is provided

// Global variables shared among threads
int numChairs;                 // Number of chairs in the waiting room
int numStudents;               // Total number of students
int leftTime, rightTime;       // Sleep interval boundaries

int *waitingRoomChairs;        // Array representing chairs (0 means empty, otherwise holds student id)
int numberStudentsWaiting = 0; // Current count of waiting students
int nextSeatingPos = 0;        // Next available seat index (circular)
int nextInterviewPos = 0;      // Next chair index from which recruiter picks a student

// Semaphores and mutex for synchronization
sem_t semStudents;     // Counts waiting students (student posts when seated)
sem_t semRecruiter;    // Used to signal a student that his/her interview is done
pthread_mutex_t mutexThread;

// Utility function: checks if the string is a positive number.
int isNumber(char number[]) {
    for (int i = 0; number[i] != '\0'; i++) {
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}

// Recruiter thread function.
// The recruiter continuously checks for waiting students. When there are none,
// the recruiter “works on his own” (sleeps for a random time). When a student is waiting,
// the recruiter removes the student from the waiting room and conducts an interview.
void* recruiter_actions(void* arg) {
    (void)arg;  // Unused parameter
    while (1) {
        // Lock to check if any student is waiting
        printf("Recruiter will call mutex_lock on mutexThread.\n");
        pthread_mutex_lock(&mutexThread);
        if (numberStudentsWaiting == 0) {
            printf("Recruiter call mutex_unlock on mutexThread.\n");
            pthread_mutex_unlock(&mutexThread);
            printf("Recruiter: No students waiting. Working on own tasks.\n");
            int workTime = mytime(leftTime, rightTime);
            printf("Recruiter to sleep %d sec; (Working on own tasks)\n", workTime);
            sleep(workTime);
            printf("Recruiter wake up; (Finished own tasks)\n");
            continue;
        }
        // At least one student is waiting; prepare to interview.
        printf("Recruiter call mutex_unlock on mutexThread.\n");
        pthread_mutex_unlock(&mutexThread);
        
        // Wait for a student to signal that they are seated.
        printf("Recruiter will call sem_wait on semStudents.\n");
        sem_wait(&semStudents);
        printf("Recruiter call sem_wait on semStudents; proceeding with interview.\n");
        
        // Lock to remove the student from the waiting room
        printf("Recruiter will call mutex_lock on mutexThread.\n");
        pthread_mutex_lock(&mutexThread);
        
        int studentId = waitingRoomChairs[nextInterviewPos];
        waitingRoomChairs[nextInterviewPos] = 0; // Remove the student from the chair
        numberStudentsWaiting--;
        nextInterviewPos = (nextInterviewPos + 1) % numChairs;
        
        printf("Recruiter starts interviewing Student %d. Students waiting = %d.\n", studentId, numberStudentsWaiting);
        
        printf("Recruiter call mutex_unlock on mutexThread.\n");
        pthread_mutex_unlock(&mutexThread);
        
        // Simulate interview time using mytime function
        int interviewTime = mytime(leftTime, rightTime);
        printf("Recruiter to sleep %d sec; (Interviewing Student %d)\n", interviewTime, studentId);
        sleep(interviewTime);
        printf("Recruiter wake up; (Finished interviewing Student %d)\n", studentId);
        
        // Signal the interviewed student that his/her interview is done.
        printf("Recruiter will call sem_post on semRecruiter for Student %d.\n", studentId);
        sem_post(&semRecruiter);
        printf("Recruiter call sem_post on semRecruiter for Student %d.\n", studentId);
    }
    printf("Recruiter %lu leaves\n", (unsigned long)pthread_self());
    return NULL;
}

// Student thread function.
// Each student alternates between studying and attempting to get an interview.
// If a chair is available in the waiting room, the student takes a seat and signals the recruiter.
// Then, the student waits until the recruiter completes the interview. After two interviews,
// the student terminates.
void* student_actions(void* arg) {
    int id = *(int*)arg;  // Student id (starting from 1)
    int interviewsDone = 0;
    
    while (interviewsDone < 2) {
        // Student is studying (programming) before attempting an interview.
        int studyTime = mytime(leftTime, rightTime);
        printf("Student %d to sleep %d sec; (Studying)\n", id, studyTime);
        sleep(studyTime);
        printf("Student %d wake up; (Finished studying)\n", id);
        
        // Student arrives at the booth.
        printf("Student %d arrives and will call mutex_lock on mutexThread.\n", id);
        pthread_mutex_lock(&mutexThread);
        
        // Check if there is an available chair.
        if (numberStudentsWaiting < numChairs) {
            // Take a seat.
            waitingRoomChairs[nextSeatingPos] = id;
            numberStudentsWaiting++;
            printf("Student %d takes a seat. Students waiting = %d.\n", id, numberStudentsWaiting);
            nextSeatingPos = (nextSeatingPos + 1) % numChairs;
            printf("Student %d call mutex_unlock on mutexThread.\n", id);
            pthread_mutex_unlock(&mutexThread);
            
            // Signal the recruiter that a student is waiting.
            printf("Student %d will call sem_post on semStudents.\n", id);
            sem_post(&semStudents);
            printf("Student %d call sem_post on semStudents.\n", id);
            
            // Wait until the recruiter completes the interview.
            printf("Student %d will call sem_wait on semRecruiter.\n", id);
            sem_wait(&semRecruiter);
            printf("Student %d call sem_wait on semRecruiter and has been interviewed.\n", id);
            
            interviewsDone++;
            printf("Student %d has completed interview %d.\n", id, interviewsDone);
        } else {
            // No chair available; leave and try later.
            printf("Student %d finds no available chairs and will try later.\n", id);
            printf("Student %d call mutex_unlock on mutexThread.\n", id);
            pthread_mutex_unlock(&mutexThread);
        }
    }
    printf("Student %d has completed two interviews and will terminate.\n", id);
    return NULL;
}

int main(int argc, char **argv) {
    // Expect exactly 4 command-line arguments:
    // <num_students> <num_chairs> <left_time> <right_time>
    if (argc != 5) {
        printf("Usage: %s <num_students> <num_chairs> <left_time> <right_time>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Validate inputs
    if (!isNumber(argv[1]) || !isNumber(argv[2]) || !isNumber(argv[3]) || !isNumber(argv[4])) {
        printf("Invalid input. All inputs must be positive integers.\n");
        exit(EXIT_FAILURE);
    }
    
    numStudents = atoi(argv[1]);
    numChairs = atoi(argv[2]);
    leftTime = atoi(argv[3]);
    rightTime = atoi(argv[4]);
    
    // Allocate waiting room chairs array and initialize to 0 (empty)
    waitingRoomChairs = malloc(numChairs * sizeof(int));
    if (waitingRoomChairs == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < numChairs; i++) {
        waitingRoomChairs[i] = 0;
    }
    
    // Initialize semaphores and mutex
    sem_init(&semStudents, 0, 0);
    sem_init(&semRecruiter, 0, 0);
    pthread_mutex_init(&mutexThread, NULL);
    
    // Seed the random number generator (for mytime function)
    srand(time(NULL));
    
    // Create the recruiter thread.
    pthread_t recruiter;
    if (pthread_create(&recruiter, NULL, recruiter_actions, NULL) != 0) {
        perror("pthread_create recruiter");
        exit(EXIT_FAILURE);
    }
    
    // Create student threads.
    pthread_t *students = malloc(numStudents * sizeof(pthread_t));
    int *studentIds = malloc(numStudents * sizeof(int));
    if (students == NULL || studentIds == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < numStudents; i++) {
        studentIds[i] = i + 1;  // Student id starts from 1
        if (pthread_create(&students[i], NULL, student_actions, (void*)&studentIds[i]) != 0) {
            perror("pthread_create student");
            exit(EXIT_FAILURE);
        }
        // Optional short sleep to stagger student arrivals.
        usleep(100000);  // 0.1 sec
    }
    
    // Join all student threads (each student terminates after two interviews)
    for (int i = 0; i < numStudents; i++) {
        pthread_join(students[i], NULL);
    }
    
    // After all student threads have terminated, cancel the recruiter thread.
    pthread_cancel(recruiter);
    pthread_join(recruiter, NULL);
    
    // Clean up resources.
    free(waitingRoomChairs);
    free(students);
    free(studentIds);
    pthread_mutex_destroy(&mutexThread);
    sem_destroy(&semStudents);
    sem_destroy(&semRecruiter);
    
    printf("All interviews completed. Program terminating.\n");
    return 0;
}
