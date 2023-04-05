#include <sys/sem.h>
/* semaphore key */
#define SEMKEY ((key_t) 400L)
/* number of semaphores being created */
#define NSEMS 1
/* GLOBAL */
int sem_id; /* semaphore id */
/* semaphore buffers */
static struct sembuf OP = {0,-1,SEM_UNDO};
static struct sembuf OV = {0,1,SEM_UNDO};
/* semapore union used to generate semaphore */
typedef union{
int val;
struct semid_ds *buf;
ushort *array;
} semunion;
/* function for semaphore to protect critical section */
int POP(){
return semop(sem_id, &OP,1);
}
/* function for semaphore to release protection */
int VOP(){
return semop(sem_id, &OV,1);
}