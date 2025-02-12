// Name: Jay Roy
// CWID: 12342760
// Project-2: Race Condition Fix
// Class: CS-300-001

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

int max;
volatile int counter = 0; // shared global variable
pthread_mutex_t lock; // mutex for protecting counter

void *mythread(void *arg)
{
    char *letter = arg;
    int i; // stack (private per thread)
    printf("%s: begin [addr of i: %p,] [addr of counter: %p]\n", letter, &i, (unsigned int)&counter);
    for (i = 0; i < max * 1000; i++)
    {
        Pthread_mutex_lock(&lock);
        counter = counter + 1; // shared: protected by mutex
        Pthread_mutex_unlock(&lock);
    }
    printf("%s: done\n", letter);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: main-first <loopcount>\n");
        exit(1);
    }
    max = atoi(argv[1]);

    pthread_t p1, p2;
    pthread_mutex_init(&lock, NULL); // initialize mutex
    
    printf("main: begin [counter = %d] [%x]\n", counter, (unsigned int)&counter);
    Pthread_create(&p1, NULL, mythread, "A");
    Pthread_create(&p2, NULL, mythread, "B");
    // join waits for the threads to finish
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: done\n [counter: %d]\n [should: %d]\n", counter, max * 2 * 1000);
    
    pthread_mutex_destroy(&lock); // destroy mutex
    return 0;
}
