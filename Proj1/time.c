/*
Name: Jay Roy
CWID - 12342760
FileName - time.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char *argv[]) {
    // Check if the command argument is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // shared memory using POSIX API with name shm_time
    char *shm_name = "/shm_time";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory segment equal to timeVal
    if (ftruncate(shm_fd, sizeof(struct timeval)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory to the process address space
    struct timeval *shared_time = mmap(NULL, sizeof(struct timeval), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_time == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Child process
    if (pid == 0) {
        // Record the starting time before executing the command
        gettimeofday(shared_time, NULL);
        // Execute the command
        printf("Child PID: %d\n", getpid());
        execvp(argv[1], argv + 1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Parent process
    else {
        // Wait for the child process to terminate
        printf("Parent PID: %d\n", getpid());
        wait(NULL);
        // Record the ending time after the child process terminates
        struct timeval end_time;
        gettimeofday(&end_time, NULL);
        // Calculate the elapsed time
        double elapsed_time = (end_time.tv_sec - shared_time->tv_sec) + (end_time.tv_usec - shared_time->tv_usec) / 1000000.0;
        // Print the elapsed time
        printf("Elapsed time: %.5f\n", elapsed_time);
    }

    // Cleanup
    munmap(shared_time, sizeof(struct timeval));
    close(shm_fd);
    shm_unlink(shm_name);
}