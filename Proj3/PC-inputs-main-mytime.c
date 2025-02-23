#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <mytime.h>

int max;
int loops;
int consumers = 1;

int *buffer;  
 
// +++++ pthread APIs: +++++ 
// pthread_mutex_t  lock1 = PTHREAD_MUTEX_INITIALIZER; 
// pthread_cond_t   cond1 = PTHREAD_COND_INITIALIZER; 
// pthread_cond_wait(&cond1, &lock1); 
// pthread_cond_signal(&cond1); 
// pthread_mutex_lock(&locl1); 
// pthread_mutex_unlock(&lock1); 

int get() { 
 
} 

void put (int c) { 	
 
} 

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

void *producer(void *arg) {
    int i = (long long int) arg;
	int t = mytime (0, 5);
	printf("Pid %d to sleep %d sec\n", i, t);
	sleep(t);
    printf("Pid %d wakeup\n", i);
	return NULL;
}

void *consumer(void *arg) {
    int i = (long long int) arg;
	int t = mytime (5, 15);
    printf("Cid %d to sleep %d sec\n", i, t);	
	sleep(t);
    printf("Cid %d wakeup\n", i);
	return NULL;
}


 int main(int argc, char *argv[]) {
    if (argc != 4) {
		fprintf(stderr, "usage: %s <buffersize> <loops> <consumers>\n", argv[0]);
		exit(1);
    }
	max = atoi(argv[1]);
    loops = atoi(argv[2]);
    consumers = atoi(argv[3]);
	
    buffer = (int *) malloc(max * sizeof(int));
    if (buffer == NULL) {printf("Allocation error!\n"); return 0;};
   
    int i;
    for (i = 0; i < max; i++) {
		buffer[i] = 0;
	}
	
   pthread_t cid[consumers];
   pthread_t p1;
   
   // semaphore init... 

   //srand (time (NULL));   
	
   printf("main: begin\n");
   pthread_create(&p1, NULL, producer, &loops); 
   for (i = 0; i < consumers; i++) {
		pthread_create(&cid[i], NULL, consumer, (void *) (long long int) i); 
   }
	
   pthread_join(p1, NULL);
   for (i = 0; i < consumers; i++) {
		pthread_join(cid[i], NULL); 
   }
   printf("main: end\n");
   return 0;
 }
