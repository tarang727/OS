#ifndef WHATEVER_H_INCLUDED
#define WHATEVER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h> /*exit()*/
#include <sys/wait.h>  /* wait(2) */
#include <unistd.h>  /* fork(2) */
#include <limits.h>	/*INT_MAX*/
#include <sys/ipc.h>  /* shmget(2), shmctl(2) */
#include <sys/shm.h>  /* shmget(2), shmat(2), shmctl(2) */
#include <sys/types.h> /* shmat(2) shmdt(2)*/
#include <signal.h>  /* catch keybord interupt */
#include <errno.h>	/*perror()*/
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */

#define MAX_STACK 10 //max number of stacks
#define MAXSIZE 5	//max size of each stack
#define NO_FIELD 6	//6 fields kept in stack_desc : the shmid, top value, no of processes currently accessing this particular stack, size of each data element, current size of the stack and no of elements in this stack (all integers)
#define public_path "/home/tarang/Desktop/os/4/global.txt"	//path for the global file to be passed to ftok(), which must be present in the same folder as all the library and header files
#define public_proj_id 999	//global proj_id to be passed to ftok(), to get the shmid of the stack descriptor
#define NO_SEM	1	// 1 global semaphore to be accessed by all processes
#define ERRNO -999	//programmer-defined error number

struct sembuf Pop;
struct sembuf Vop;

#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);

key_t public_key; //universal key to be used by all
int semid; //universal semaphore to be used by all

//This function is to initialize the semaphore set. It should be called at the beginning of the main() function
void sem_init();

//This function is to create/get the id of stack_descriptor in shared memory. The first to call this will create the stack descriptor, and others will use it. 
int shstack_desc_get();

//This function takes (shared memory id of stack descriptor, user id of stack to be accessed by user)
//returns the shared memory id of the stack created
int shstackget(int , int);

//This function takes (shared memory id of stack descriptor,shared memory id of stack to be accessed by user, user id of stack to be accessed by user, and address of status)
//returns the value popped
//pops value at top from the stack
int shstackpop(int, int, int, int*);
//This function takes (shared memory id of stack descriptor,shared memory id of stack to be accessed by user, user id of stack to be accessed by user, data to be pushed and address of status)
//returns nothing
//pushes element into the stack
void shstackpush(int, int, int, int, int*);

//This function takes (shared memory id of stack descriptor,shared memory id of stack to be accessed by user, user id of stack to be accessed by user, and address of status)
//returns nothing
//displays the stack
void shstackdisplay(int, int, int, int*);

//This function takes (shared memory id of stack descriptor,user id of stack to be accessed by user, and address of status)
//returns nothing
//destroys the stack
//sets status accordingly
void shstackrm(int,int,int*);

//This function takes (shared memory id of stack descriptor,user id of stack to be accessed by user)
//returns nothing
//increments the value in stack descriptor denoting the no. of processes using it
//sets status accordingly
void shstack_busy_on(int,int);

//This function takes (shared memory id of stack descriptor,user id of stack to be accessed by user)
//returns nothing
//decrements the value in stack descriptor denoting the no. of processes using it
//sets status accordingly
void shstack_busy_off(int,int);

//This function takes (shared memory id of stack descriptor)
//returns nothing
//displays the present status of the stack descriptor in tabular format
void shstackdescInfo(int);

#endif
