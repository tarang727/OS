/*

This is the .c file associated with the sharedstack.h header file. This file contains all the definitions of the functions provided in sharedstack.h header file.
The stack descriptor can be assumed to be a struct as follows:

struct shared_stack_desc{
	int shmid;	//shared memory id of each stack
	int top;	//index of top element
	int no_of_processes; //no of processes currently accessing this particular stack
	int data_size;	//size of each data element
	int stack_size;	//current size of the stack
	int no_of_element;	//no of elements in this stack
};

*/

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
#include "sharedstack.h"

void sem_init() {	//This function is to initialize the semaphore to be used by all processes
	public_key=ftok(public_path,public_proj_id);

	union semun {
		int              val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
	} setvalArg;

	setvalArg.val = 1;

	Pop.sem_num = 0;
	Pop.sem_op = -1;
	Pop.sem_flg = SEM_UNDO;

	Vop.sem_num = 0;
	Vop.sem_op = 1;
	Vop.sem_flg = SEM_UNDO;

	semid = semget(public_key, NO_SEM, IPC_CREAT | 0777);
	if(semid == -1) {
		perror("semget() failed");
		exit(1);
	}

	if(semctl(semid, 0, SETVAL, setvalArg)==-1) {
		perror("semctl() failed");
		exit(1);
	}

}

int shstack_desc_get()	{	//func to create or get the shmid of the stack_descriptor
	int shmid;
	public_key=ftok(public_path,public_proj_id);
	P(semid);
	shmid=shmget(public_key, NO_FIELD*MAX_STACK*sizeof(int),IPC_EXCL| IPC_CREAT | 0777);
	V(semid);

	if(shmid==-1)
	{
		if(errno==EEXIST)
		{
			shmid=shmget(public_key, NO_FIELD*MAX_STACK*sizeof(int),0);
		}
	}
	else
	{
		int *pi=(int*)shmat(shmid,NULL,0);
		for(int i=0;i<NO_FIELD*MAX_STACK;i+=NO_FIELD)
		{
			pi[i]=0;
			pi[i+1]=-1;
			pi[i+2]=0;
			pi[i+3]=sizeof(int);
			pi[i+4]=0;
			pi[i+5]=0;
		}
		shmdt(pi);
	} 
	return(shmid);
}

int shstackget(int shstack_desc_id, int id) {	//function to create a stack in shared memory and get the shmid of the stack to be created
	int st_id;
	key_t st_key=ftok(public_path, (id+1));
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[NO_FIELD*id]==0)
	{
		P(semid);
		printf("\nNo such stack exists of now.\n");
		st_id=shmget(st_key,MAXSIZE*sizeof(int),IPC_CREAT|0777);
		printf("\nNew stack created with shmid : %d\n",st_id);
		pi[NO_FIELD * id]=st_id;
		V(semid);
	}
	else
	{
		P(semid);
		printf("\nStack has already been created.\n");
		st_id=pi[NO_FIELD * id];
		V(semid);
	}
	shmdt(pi);
	return(st_id);

}

int shstackpop(int shstack_desc_id, int st_shmid, int id, int *status) {	//pop value from shared stack
	int data=INT_MAX;
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[NO_FIELD*id]==0)
	{
		printf("\nSorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
		*status=ERRNO;
		shmdt(pi);
		return *status;
	}
	else
	{
		P(semid);

		int *stack_pi=(int*)shmat(st_shmid,NULL,0);
		int top=pi[(NO_FIELD*id)+1];
		if(top!=-1) {
		  data = stack_pi[top];
		  pi[(NO_FIELD*id)+4]-=sizeof(data);
		  pi[(NO_FIELD*id)+5]--;
		  top--;
		} else {
		  printf("\nCould not retrieve data, Stack is empty.\n");
		}
		pi[(NO_FIELD*id)+1]=top;
		V(semid);
		shmdt(stack_pi);
		shmdt(pi);
		*status=1;
		return data;		

	}
}

void shstackpush(int shstack_desc_id, int st_shmid, int id, int data, int *status) {	//push value in shared stack
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[NO_FIELD*id]==0)
	{
		printf("\nSorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
		*status=ERRNO;
		shmdt(pi);
	}
	else
	{
		P(semid);
		int *stack_pi=(int*)shmat(st_shmid,NULL,0);
		int top=pi[(NO_FIELD*id)+1];
		if(top!=(MAXSIZE-1)) {
		  top++;   
		  stack_pi[top] = data;
		  pi[(NO_FIELD*id)+4]+=sizeof(data);
		  pi[(NO_FIELD*id)+5]++;
		  printf("\n%d has been successfully pushed into stack.\n",data);
		} else {
		  printf("\nCould not insert data, Stack is full.\n");
		}
		pi[(NO_FIELD*id)+1]=top;
		V(semid);
		shmdt(stack_pi);
		shmdt(pi);
		*status=1;		

	}
}

void shstackdisplay(int shstack_desc_id, int st_shmid, int id, int *status) {
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[NO_FIELD*id]==0)
	{
		printf("\nSorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
		*status=ERRNO;
		shmdt(pi);
	}
	else
	{
		P(semid);
		int *stack_pi=(int*)shmat(st_shmid,NULL,0);
		int top=pi[(NO_FIELD*id)+1];
		if(top!=-1) {
			printf("\n-------------------DISPLAY------------------\n");
			for(int i=top;i>=0;i--)
				printf(" -> %d",stack_pi[i]);
		}
		else
			printf("\tSTACK IS EMPTY\t");
		V(semid);
		printf("\n");
		*status=1;
		shmdt(stack_pi);
		shmdt(pi);
	}	
}

void shstackrm(int shstack_desc_id, int id, int *status) {
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[(NO_FIELD*id)+2]>0)
	{
		printf("\nThis stack cannot be removed as it is currently used by some process.\nPlease try again later...\n");
		*status=ERRNO;
		shmdt(pi);
	}
	else
	{
		if(pi[NO_FIELD*id]!=0)
		{
			P(semid);
			shmctl(pi[NO_FIELD*id],0,IPC_RMID);
			pi[NO_FIELD*id]=0;
			pi[(NO_FIELD*id)+1]=-1;
			pi[(NO_FIELD*id)+2]=0;
			pi[(NO_FIELD*id)+3]=sizeof(int);
			pi[(NO_FIELD*id)+4]=0;
			pi[(NO_FIELD*id)+5]=0;
			printf("\nStack has been successfully destroyed.\n");
			V(semid);
		}
		else
			printf("\nNo stack exists to be destroyed.\n");
		*status=1;
		shmdt(pi);
	}

}

void shstack_busy_on(int shstack_desc_id, int id) {

	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	P(semid);
	pi[(NO_FIELD*id)+2]++;
	V(semid);
	shmdt(pi);
}

void shstack_busy_off(int shstack_desc_id, int id) {

	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	P(semid);
	if(pi[(NO_FIELD*id)+2]>0)
	pi[(NO_FIELD*id)+2]--;
	V(semid);
	shmdt(pi);
}

void shstackdescInfo(int shstack_desc_id) {
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	printf("\n----------------------------------------------------------------------Shared stack descriptor-------------------------------------------------------------\n\n");
	int count=0;
	for(int i=0;i<NO_FIELD*MAX_STACK;i+=NO_FIELD)
	{
		P(semid);
		printf("Id = %d\tShmid = %d\tTop value = %d\tNo of processes using this stack = %d\tData size = %d bytes\tStack size = %d bytes\tNo. of elements = %d\n\n",count++,pi[i],pi[i+1],pi[i+2],pi[i+3],pi[i+4],pi[i+5]);
		V(semid);
	}
	shmdt(pi);

}