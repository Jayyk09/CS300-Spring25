
/*Name: Jay Roy
CWID: 12342760
P3: Jay Roy
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include "mytime.h"

int students;
int chairs;
int left;
int right;
int waiting = 0;
sem_t recruiter_ready;
sem_t students_waiting;
 
// +++++ pthread APIs: +++++ 
pthread_mutex_t  student_lock = PTHREAD_MUTEX_INITIALIZER; 
// pthread_cond_t   cond1 = PTHREAD_COND_INITIALIZER; 
// pthread_cond_wait(&cond1, &lock1); 
// pthread_cond_signal(&cond1); 
// pthread_mutex_lock(&locl1); 
// pthread_mutex_unlock(&lock1); 

// int get() { 
//     int temp = buffer[use];
//     use = (use + 1) % max;
//     return temp;
// } 

// void put (int c) { 	
//     buffer[fill] = c;
//     fill = (fill + 1) % max;
// }

/*int mysleep (int left, int right)
{
	int mytime = 0;
	int n = 0;

	n = rand()%(right - left);	
	mytime = left + n;	
	printf("random time is %d sec\n", mytime);
	return  mytime;
}
*/


void *recruiter(void *arg) {
    while (1) {
        if (waiting == 0) {
            printf("Recruiter is idle, will go back to work.\n");
        }
        printf("Recruiter will call sem_wait(students_waiting)\n");
        sem_wait(&students_waiting);
        printf("Recruiter will call mutex_lock\n");
        pthread_mutex_lock(&student_lock);
        waiting--;
        pthread_mutex_unlock(&student_lock);
        printf("Recruiter call mutex_unlock\n");
        sem_post(&recruiter_ready);
        printf("Recruiter call sem_post(recruiter_ready)\n");
        int t = mytime(left, right);
        printf("Recruiter to sleep %d sec;\n", t);
        sleep(t);
        printf("Recruiter Id %lu wake up;\n", pthread_self());
    }
    printf("Recruiter Id %lu exit;\n", pthread_self());
    return NULL;
}

void *student(void *arg) {
    int id = (long long int)arg;
    for (int i = 0; i < 2; i++) {
        int t = mytime(left, right);
        printf("Student %d to sleep %d sec;\n", id, t);
        sleep(t);
        printf("Student Id %d wake up;\n", id);
        printf("Student %d will call mutex_lock\n", id);
        pthread_mutex_lock(&student_lock);
        if (waiting < chairs) {
            waiting++;
            printf("Student %d waiting, %d chairs left.\n", id, chairs - waiting);
            pthread_mutex_unlock(&student_lock);
            printf("Student %d call mutex_unlock\n", id);
            sem_post(&students_waiting);
            printf("Student %d call sem_post(students_waiting)\n", id);
            sem_wait(&recruiter_ready);
            printf("Student %d call sem_wait(recruiter_ready)\n", id);
            printf("Student %d being interviewed.\n", id);
        } else {
            pthread_mutex_unlock(&student_lock);
            printf("Student %d call mutex_unlock\n", id);
            printf("Student %d found no available chairs, will study and return.\n", id);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <students> <chairs> <left> <right>\n", argv[0]);
        exit(1);
    }
    students = atoi(argv[1]);
    chairs = atoi(argv[2]);
    left = atoi(argv[3]);
    right = atoi(argv[4]);

    if (students <= 0 || chairs <= 0 || left <= 0 || right <= 0 || left >= right) {
        fprintf(stderr, "Invalid input: students, chairs, left, right must be positive integers and left must be less than right.\n");
        exit(1);
    }

    srand(time(NULL));
    sem_init(&recruiter_ready, 0, 0);
    sem_init(&students_waiting, 0, 0);

    pthread_t student_tids[students];
    pthread_t recruiter_tid;
    pthread_create(&recruiter_tid, NULL, recruiter, NULL);

    for (int i = 0; i < students; i++) {
        pthread_create(&student_tids[i], NULL, student, (void *)(long long int)(i + 1));
    }

    for (int i = 0; i < students; i++) {
        pthread_join(student_tids[i], NULL);
    }
    pthread_cancel(recruiter_tid);

    printf("All students interviewed twice. Exiting.\n");
    return 0;
}