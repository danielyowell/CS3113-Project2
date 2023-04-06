#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/ipc.h>

// * SHARED MEMORY

/* key number */
#define SHMKEY ((key_t) 1497)
/* defines struct shared_mem with int variable "value" */
typedef struct
{
    int value;
} shared_mem;
/* create shared memory (pointer) */
shared_mem *total;

// * SEMAPHORES

/* semaphore key */
#define SEMKEY ((key_t) 400L)

/* number of semaphores being created */
#define NSEMS 1

/* GLOBAL */
int semaphore_id; /* semaphore id */

/* semaphore buffers */
static struct sembuf OP = {0,-1,SEM_UNDO};
static struct sembuf OV = {0,1,SEM_UNDO};

/* union to store semaphore value (I think struct also works) */
typedef union {
  int val;
  struct semid_ds *buf;
  ushort *array;
} semunion;

/* function for semaphore to protect critical section */
int POP(){
return semop(semaphore_id, &OP,1);
}

/* function for semaphore to release protection */
int VOP(){
return semop(semaphore_id, &OV,1);
}

// * PROCESSES
void process1() {
    for(int x = 0; x < 100000; x++) {
        total->value = total->value + 1;
    }
    return;
}
void process2() {
    for(int x = 0; x < 200000; x++) {
        total->value = total->value + 1;
    }
    return;
}
void process3() {
    for(int x = 0; x < 300000; x++) {
        total->value = total->value + 1;
    }
    return;
}
void process4() {
    for(int x = 0; x < 500000; x++) {
        total->value = total->value + 1;
    }
    return;
}

int main(void) {
  
  // * SHARED MEMORY INITIALIZATION
  int shmid = shmget (SHMKEY, sizeof(int), IPC_CREAT | 0666);
  /* main function address */
    char *shmadd;
    shmadd = (char *) 0;
    
    total = (shared_mem*) shmat (shmid, shmadd, 0);
    
    total->value = 0;

  // * SEMAPHORE INITIALIZATION
  int status;
  pid_t pid;
  int cpid;
  struct sembuf semaphore;
  // Create a semaphore with an initial value of 1.
  semaphore_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
  if (semaphore_id < 0) {
    perror("Could not create semaphore");
    exit(1);
  }
  semunion su;
  su.val = 1;
  status = semctl(semaphore_id, 0, SETVAL, su);
  if (status < 0) {
    perror("Could not set semaphore value");
    exit(1);
  }

  pid = fork();
    // CHILD 1
    if(pid == 0) { 
        // WAITING
        POP();
        // CRITICAL
        process1(); 
        printf("From Process 1: counter = %d.\n", total->value);
        // EXITING
        VOP();
        exit(0);
    }
    // PARENT
    if(pid != 0) {
        pid = fork();
        // CHILD 2
        if(pid == 0) { 
            // WAITING
            POP();
            // CRITICAL
            process2(); 
            printf("From Process 2: counter = %d.\n", total->value);
            // EXITING
            VOP();
            exit(0); 
        }
        // PARENT
        if(pid != 0) {
            pid = fork();
            // CHILD 3
            if(pid == 0) { 
                // WAITING
                POP();
                // CRITICAL
                process3(); 
                printf("From Process 3: counter = %d.\n", total->value);
                // EXITING
                VOP();
                exit(0);
            }
            // PARENT
            if(pid != 0) {
                pid = fork();
                // CHILD 4
                if(pid == 0) { 
                    // WAITING
                    POP();
                    // CRITICAL
                    process4(); 
                    printf("From Process 4: counter = %d.\n", total->value);
                    // EXITING
                    VOP();
                    exit(0);
                }
                // PARENT
                if(pid != 0) {
                    cpid = wait(NULL);
                    printf("Child with ID: %d has just exited.\n", cpid);
                    cpid = wait(NULL);
                    printf("Child with ID: %d has just exited.\n", cpid);
                    cpid = wait(NULL);
                    printf("Child with ID: %d has just exited.\n", cpid);
                    cpid = wait(NULL);
                    printf("Child with ID: %d has just exited.\n", cpid);
                    // detach shared memory
                    if (shmdt(total) == -1) {
                        perror ("shmdt error (could not detach shared memory)");
                        exit (-1);
                    }   
                    // delete shared memory
                    shmctl(shmid, IPC_RMID, NULL); 

                    // Remove the semaphore.
                    status = semctl(semaphore_id, 0, IPC_RMID);
                    if (status == -1) {
                        perror("semctl error (could not remove semaphore)");
                        exit(1);
                    }

                    printf("\nEnd of simulation.\n");
                }                    
            }
        }
    }
      return 0;
  }
