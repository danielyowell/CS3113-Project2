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

int main(void) {
  // SHARED MEMORY
  int shmid = shmget (SHMKEY, sizeof(int), IPC_CREAT | 0666);
  /* main function address */
    char *shmadd;
    shmadd = (char *) 0;
    
    total = (shared_mem*) shmat (shmid, shmadd, 0);
    
    total->value = 0;

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

  // Create child processes.
  for (int i = 0; i < MAX_THREADS; i++) {
    pid = fork();
    // safety code
    if (pid < 0) {
      perror("Error creating child process");
      exit(1);
    } 

    // child process
    if (pid == 0) {
      printf("Child %d is waiting...\n", i);
      semaphore.sem_num = 0;
      semaphore.sem_op = -1;
      semaphore.sem_flg = SEM_UNDO;
      semop(semaphore_id, &semaphore, 1);

      printf("Child %d is in critical section.\n", i);
      for(int i = 0; i < 100; i++) {
        total->value = total->value + 1;
      }

      semaphore.sem_op = 1;
      semop(semaphore_id, &semaphore, 1);
      printf("Child %d is exiting.\n", i);

      exit(0);
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
