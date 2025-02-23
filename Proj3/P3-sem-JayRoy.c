#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "mytime.h"

void *student(void *num);
void *recruiter(void *);

// Semaphores for waiting room chairs, interview chair, interview completion, and student readiness.
sem_t waitingRoom;      
sem_t recruiterChair;     
sem_t seatBelt;           
sem_t studentReady;       

pthread_mutex_t mutex;    // For protecting shared variables.

int studentsFinished = 0;
int numStudents, numChairs, left, right;

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <num_students> <num_chairs> <left> <right>\n", argv[0]);
        return -1;
    }

    srand(time(NULL)); // Set random seed

    numStudents = atoi(argv[1]);
    numChairs = atoi(argv[2]);
    left = atoi(argv[3]);
    right = atoi(argv[4]);

    pthread_t recruiterThread;
    pthread_t studentThreads[numStudents];
    int studentNumbers[numStudents];

    // Initialize semaphores.
    sem_init(&waitingRoom, 0, numChairs);
    sem_init(&recruiterChair, 0, 1);
    sem_init(&seatBelt, 0, 0);
    sem_init(&studentReady, 0, 0);

    pthread_mutex_init(&mutex, NULL);

    // Start the recruiter thread.
    pthread_create(&recruiterThread, NULL, recruiter, NULL);

    // Create student threads.
    for (int i = 0; i < numStudents; i++) {
        studentNumbers[i] = i + 1;
        pthread_create(&studentThreads[i], NULL, student, (void *)&studentNumbers[i]);
        sleep(1);  // Stagger student creation.
    }

    // Wait for all student threads to complete.
    for (int i = 0; i < numStudents; i++) {
        pthread_join(studentThreads[i], NULL);
    }

    // Signal that students are finished.
    studentsFinished = 1;
    // Post to studentReady to unblock recruiter if waiting.
    sem_post(&studentReady);
    pthread_join(recruiterThread, NULL);

    return 0;
}

void *student(void *number) {
    int id = *(int *)number;
    for (int i = 0; i < 2; i++) {  // Each student gets 2 interview opportunities.
        int sleepTime = mytime(left, right);
        printf("Student %d to sleep for %d seconds.\n", id, sleepTime);
        fflush(stdout);
        sleep(sleepTime);

        // Try to enter the waiting room.
        sem_wait(&waitingRoom);
        printf("Student %d takes a chair.\n", id);
        fflush(stdout);

        // Wait for the interview chair.
        sem_wait(&recruiterChair);

        // Signal the recruiter that a student is ready.
        sem_post(&studentReady);

        // Wait for the interview to finish.
        sem_wait(&seatBelt);

        // Release the interview chair and waiting room chair.
        sem_post(&recruiterChair);
        sem_post(&waitingRoom);
    }
    return NULL;
}

void *recruiter(void *junk) {
    while (1) {
        sem_wait(&studentReady); // Wait until a student is ready.
        pthread_mutex_lock(&mutex);
        if (studentsFinished) {
            pthread_mutex_unlock(&mutex);
            break;  // Exit if all students have finished.
        }
        pthread_mutex_unlock(&mutex);

        int sleepTime = mytime(left, right);
        sleep(sleepTime);  // Simulate interview duration.
        printf("Recruiter finished an interview.\n");
        fflush(stdout);

        // Signal the student that the interview is over.
        sem_post(&seatBelt);
    }
    printf("Recruiter has exited.\n");
    fflush(stdout);
    return NULL;
}
