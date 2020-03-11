#ifndef WHATEVER_H_INCLUDED
#define WHATEVER_H_INCLUDED

/*

This is the header file of the generic shared stack implementation so that multiple processes can work on these shared stacks. It contains all the functions required for the implementation.
The stack descriptor can be assumed to be a struct as follows:

struct shared_stack_desc{
	int shmid;	//shared memory id of each stack
	int top;	//index of top element
	int no_of_processes; //no of processes currently accessing this particular stack
	int data_size;	//size of each data element
	int stack_size;	//max size of this stack
	int no_of_element;	//no of elements in this stack presently
	int type;	//type of this stack : 1 for char, 2 for int, 3 for float, 4 for double
};

*/

#include <stdio.h>
#include <stdlib.h> /*exit()*/
#include <unistd.h> 
#include <limits.h>	/*INT_MAX*/
#include <sys/ipc.h>  /* shmget(2), shmctl(2) */
#include <sys/shm.h>  /* shmget(2), shmat(2), shmctl(2) */
#include <sys/types.h> /* shmat(2) shmdt(2)*/
#include <errno.h>	/*perror()*/
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */

#define MAX_STACK 10 //max number of stacks
#define NO_FIELD 7	//7 fields kept in stack_desc : the shmid, top value, no of processes currently accessing this particular stack, size of each data element, current size of the stack and no of elements in this stack (all integers)
#define public_path "/home/tarang/Desktop/os/4/global.txt"	//path for the global file to be passed to ftok(), which must be present in the same folder as all the library and header files
#define public_proj_id 999	//global proj_id to be passed to ftok(), to get the shmid of the stack descriptor
#define NO_SEM	1	// 1 global semaphore to be accessed by all processes
#define ERRNO -999	//programmer-defined error number


/*associated structs and functions for the semaphore set has been declared globally so that all functions can freely use them*/

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

//This function takes (shared memory id of stack descriptor, user id of stack to be accessed by user) as input and returns the shared memory
//returns id of the stack created
//creates a stack in shared memory and get the shmid of the stack which has been created
int shstackget(int , int);

//This function takes (shared memory id of stack descriptor,shared memory id of stack to be accessed by user, user id of stack to be accessed by user, and address of status)
//returns the value popped
//pops and returns the value from the top of the stack
void* shstackpop(int, int, int, int*);

//This function takes (shared memory id of stack descriptor,shared memory id of stack to be accessed by user, user id of stack to be accessed by user, pointer to data to be pushed and address of status)
//returns nothing
//pushes the value into the respective stack
void shstackpush(int, int, int, void*, int*);

//This function takes (shared memory id of stack descriptor,shared memory id of stack to be accessed by user, user id of stack to be accessed by user, and address of status)
//returns nothing
//displays the stack
void shstackdisplay(int, int, int, int*);

//This function takes (shared memory id of stack descriptor,user id of stack to be accessed by user, and address of status)
//returns nothing
//destroys the stack (only if no other)
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

//This function takes (shared memory id of stack descriptor,user id of stack to be accessed by user)
//returns an integer (1 for char, 2 for int, 3 for float, 4 for double, 0 if stack not initialized yet)
//returns the type of the respective stack
int getshstackType(int, int);

#endif
