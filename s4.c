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

/* key number */
#define SHMKEY ((key_t) 1497)

/* defines struct shared_mem with int variable "value" */
typedef struct
{
    int value;
} shared_mem;

/* create shared memory (pointer) */
shared_mem *total;

#define MAX_THREADS 4

// Union to store the value of the semaphore.
union semun {
  int val;
  struct semid_ds *buf;
  ushort *array;
} argument;

// PROCESSES
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
  int semaphore_id, status;
  pid_t pid;
  struct sembuf semaphore;
  // Create a semaphore with an initial value of 1.
  semaphore_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
  if (semaphore_id < 0) {
    perror("Error creating semaphore");
    exit(1);
  }
  argument.val = 1;
  status = semctl(semaphore_id, 0, SETVAL, argument);
  if (status < 0) {
    perror("Error setting semaphore value");
    exit(1);
  }

  // * FORK CHILD PROCESSES
  for (int i = 1; i <= MAX_THREADS; i++) {
    pid = fork();
    // safety code
    if (pid < 0) {
      perror("Error creating child process");
      exit(1);
    } 

    // child process
    if (pid == 0) {
      
      // WAITING
        semaphore.sem_num = 0;
        semaphore.sem_op = -1;
        semaphore.sem_flg = SEM_UNDO;
        semop(semaphore_id, &semaphore, 1); 
      
      // CRITICAL SECTION
        if(i==1) {
            process1();
            printf("From Process 1: counter = %d.\n", total->value);
        }
        if(i==2) {
            process2();
            printf("From Process 2: counter = %d.\n", total->value);
        }
        if(i==3) {
            process3();
            printf("From Process 3: counter = %d.\n", total->value);
        }
        if(i==4) {
            process4();
            printf("From Process 4: counter = %d.\n", total->value);
        }
      
      // EXITING
        semaphore.sem_op = 1;
        semop(semaphore_id, &semaphore, 1);
        exit(0);
    }

    // parent process
    if(pid != 0) {
        int cpid = wait(NULL);
        printf("Child with ID: %d has just exited.\n", cpid);
    }
  }

  // Wait for child processes to finish.
  for (int i = 0; i < MAX_THREADS; i++) {
    wait(NULL);
  }

  // Remove the semaphore.
  status = semctl(semaphore_id, 0, IPC_RMID);
  if (status < 0) {
    perror("Error removing semaphore");
    exit(1);
  }

  // detach shared memory
    if (shmdt(total) == -1) {
        perror ("shmdt");
        exit (-1);
    }   
    // delete shared memory
    shmctl(shmid, IPC_RMID, NULL); 

  return 0;
}
