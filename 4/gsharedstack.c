/*

This is the .c file associated with the gsharedstack.h header file. This file contains all the definitions of the functions provided in gsharedstack.h header file.
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
#include <sys/wait.h>  /* wait(2) */
#include <unistd.h>  /* fork(2) */
#include <limits.h>	/*INT_MAX*/
#include <sys/ipc.h>  /* shmget(2), shmctl(2) */
#include <sys/shm.h>  /* shmget(2), shmat(2), shmctl(2) */
#include <sys/types.h> /* shmat(2) shmdt(2)*/
#include <signal.h>  /* catch keybord interupt */
#include <errno.h>	/*perror()*/
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */
#include "gsharedstack.h"

void sem_init() {	//This function is to initialize the semaphore to be used by all processes
	
	public_key=ftok(public_path,public_proj_id);	//global key to be used by all processes


	/*Initializing the semaphore set*/

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

	//getting the id of the semaphore
	semid = semget(public_key, NO_SEM, IPC_CREAT | 0777);
	if(semid == -1) {
		perror("semget() failed");
		exit(1);
	}

	//setting the value for the semaphore set
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
		if(errno==EEXIST)	//that means some process has already created the stack descriptor
		{
			shmid=shmget(public_key, NO_FIELD*MAX_STACK*sizeof(int),0);	//only the id is acquired
		}
	}
	else
	{
		int *pi=(int*)shmat(shmid,NULL,0);	//attaching the stack descriptor to the current process
		for(int i=0;i<NO_FIELD*MAX_STACK;i+=NO_FIELD)
		{
			/*Initializing all the fields*/
			pi[i]=0;
			pi[i+1]=-1;
			pi[i+2]=0;
			pi[i+3]=0;
			pi[i+4]=0;
			pi[i+5]=0;
			pi[i+6]=0;
		}
		shmdt(pi);	//detaching the stack descriptor
	} 
	return(shmid);
}

int shstackget(int shstack_desc_id, int id) {

	int st_id,type,ch,element_size,stack_size;
	key_t st_key=ftok(public_path, (id+1));
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);	//attaching the stack descriptor 

	if(pi[NO_FIELD*id]==0)	//The stack denoted by id has not been created yet
	{
		P(semid);
		printf("\nNo such stack exists of now.\n");
		printf("Enter size of this stack (no. of elements): ");
		scanf("%d",&stack_size);
		printf("\n1.Char\n2.Int\n3.Float\n4.Double\nEnter the data type : ");
		scanf("%d",&ch);
		if(ch==1)
			element_size=sizeof(char);	//setting the size of the data element
		else if(ch==2)
			element_size=sizeof(int);
		else if(ch==3)
			element_size=sizeof(float);
		else
			element_size=sizeof(double);
		st_id=shmget(st_key,stack_size*element_size,IPC_CREAT|0777);	//creating the stack of given size and type
		printf("\nNew stack created with shmid : %d\n",st_id);
		pi[NO_FIELD * id]=st_id;	//storing the shared memory id of the stack created
		pi[(NO_FIELD*id)+3]=element_size;	//storing the size of each data element
		pi[(NO_FIELD*id)+4]=stack_size;	//storing the user-defined size of each stack 
		pi[(NO_FIELD*id)+6]=ch;	//storing the type of stack chosen
		V(semid);
	}
	else
	{
		P(semid);
		printf("\nStack has already been created.\n");
		st_id=pi[NO_FIELD * id];	//just getting the shared memory id from the stack descriptor
		V(semid);
	}
	shmdt(pi);
	return(st_id);

}

void* shstackpop(int shstack_desc_id, int st_shmid, int id, int *status) {	//pop value from shared stack
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	void* data=(void*)NULL;	//the value popped will be returned as a void pointer
	if(pi[NO_FIELD*id]==0)	//this means the stack has been suddenly destroyed
	{
		//This part actually provides a second layer of protection to the shared stacks, since a process cannot remove a stack if some other process is still using it.
		//show appropriate error message
		printf("\nSorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
		*status=ERRNO;	//setting the status accordingly
		shmdt(pi);
		return (void*)NULL;
	}
	else
	{
		int type=getshstackType(shstack_desc_id, id);	//getting the type of stack
		if(type==1)	//char
		{
			P(semid);
			char *stack_pi=(char*)shmat(st_shmid,NULL,0);
			char val;
			int top=pi[(NO_FIELD*id)+1];
			if(top!=-1) {
			  val=stack_pi[top];
			  data=(void*)&val;
			  pi[(NO_FIELD*id)+5]--;
			  top--;
			} else {
			  printf("\nCould not retrieve data, Stack is empty.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);
		}
		else if(type==2)	//int
		{
			P(semid);
			int *stack_pi=(int*)shmat(st_shmid,NULL,0);	//attaching the current stack
			int val;
			int top=pi[(NO_FIELD*id)+1];	//getting the top index
			if(top!=-1) {
			  val=stack_pi[top];
			  data=(void*)&val;
			  pi[(NO_FIELD*id)+5]--;	//decreasing the no. of elements in this stack
			  top--;
			} else {
			  printf("\nCould not retrieve data, Stack is empty.\n");
			}
			pi[(NO_FIELD*id)+1]=top;	//storing the final top index after popping
			V(semid);
			shmdt(stack_pi);	//detaching stack_pi
		}
		else if(type==3)	//float
		{
			P(semid);
			float *stack_pi=(float*)shmat(st_shmid,NULL,0);
			float val;
			int top=pi[(NO_FIELD*id)+1];
			if(top!=-1) {
			  val=stack_pi[top];
			  data=(void*)&val;
			  pi[(NO_FIELD*id)+5]--;
			  top--;
			} else {
			  printf("\nCould not retrieve data, Stack is empty.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);
		}
		else if(type==4)	//double
		{
			P(semid);
			double *stack_pi=(double*)shmat(st_shmid,NULL,0);
			double val;
			int top=pi[(NO_FIELD*id)+1];
			if(top!=-1) {
			  val=stack_pi[top];
			  data=(void*)&val;
			  pi[(NO_FIELD*id)+5]--;
			  top--;
			} else {
			  printf("\nCould not retrieve data, Stack is empty.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);
		}
		else//no type (type=0)
		{
			printf("No type selected.");
		}
		shmdt(pi);
		*status=1;
		return data;		

	}
}

void shstackpush(int shstack_desc_id, int st_shmid, int id, void* data, int *status) {	//push value in shared stack. The value is taken as a void pointer data.
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[NO_FIELD*id]==0)
	{
		printf("\nSorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
		*status=ERRNO;	//setting the status accordingly
		shmdt(pi);
	}
	else
	{
		int type=getshstackType(shstack_desc_id, id);
		int MAXSIZE=pi[(NO_FIELD*id)+4];	//getting the max size of this stack
		if(type==1)	//char
		{
			P(semid);
			char *stack_pi=(char*)shmat(st_shmid,NULL,0);
			char val= *((char*)data);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=(MAXSIZE-1)) {
			  top++;   
			  stack_pi[top] = val;
			  pi[(NO_FIELD*id)+5]++;
			  printf("\n%c has been successfully pushed into stack.\n",val);
			} else {
			  printf("\nCould not insert data, Stack is full.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);
		}
		else if(type==2)	//int
		{
			P(semid);
			int *stack_pi=(int*)shmat(st_shmid,NULL,0);
			int val= *((int*)data);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=(MAXSIZE-1)) {
			  top++;   
			  stack_pi[top] = val;
			  pi[(NO_FIELD*id)+5]++;
			  printf("\n%d has been successfully pushed into stack.\n",val);
			} else {
			  printf("\nCould not insert data, Stack is full.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);
		}
		else if(type==3)	//float
		{
			P(semid);
			float *stack_pi=(float*)shmat(st_shmid,NULL,0);
			float val= *((float*)data);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=(MAXSIZE-1)) {
			  top++;   
			  stack_pi[top] = val;
			  pi[(NO_FIELD*id)+5]++;
			  printf("\n%f has been successfully pushed into stack.\n",val);
			} else {
			  printf("\nCould not insert data, Stack is full.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);			
		}
		else if(type==4)	//double
		{
			P(semid);
			double *stack_pi=(double*)shmat(st_shmid,NULL,0);
			double val= *((double*)data);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=(MAXSIZE-1)) {
			  top++;   
			  stack_pi[top] = val;
			  pi[(NO_FIELD*id)+5]++;
			  printf("\n%lf has been successfully pushed into stack.\n",val);
			} else {
			  printf("\nCould not insert data, Stack is full.\n");
			}
			pi[(NO_FIELD*id)+1]=top;
			V(semid);
			shmdt(stack_pi);
		}
		else 	//no type
		{
			printf("No type selected.");
		}			
		shmdt(pi);
		*status=1;		

	}
}

void shstackdisplay(int shstack_desc_id, int st_shmid, int id, int *status) {
	/*function to display the stack*/
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[NO_FIELD*id]==0)
	{
		printf("\nSorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
		*status=ERRNO;	//setting the status accordingly
		shmdt(pi);
	}
	else
	{
		int type=getshstackType(shstack_desc_id, id);
		if(type==1)
		{
			P(semid);
			char *stack_pi=(char*)shmat(st_shmid,NULL,0);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=-1) {
				printf("\n-------------------DISPLAY------------------\n");
				for(int i=top;i>=0;i--)
					printf(" -> %c",stack_pi[i]);
			}
			else
				printf("\n\tSTACK IS EMPTY\t");
			V(semid);
			printf("\n");
			shmdt(stack_pi);
		}
		else if(type==2)
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
				printf("\n\tSTACK IS EMPTY\t");
			V(semid);
			printf("\n");
			shmdt(stack_pi);
		}
		else if(type==3)
		{
			P(semid);
			float *stack_pi=(float*)shmat(st_shmid,NULL,0);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=-1) {
				printf("\n-------------------DISPLAY------------------\n");
				for(int i=top;i>=0;i--)
					printf(" -> %f",stack_pi[i]);
			}
			else
				printf("\n\tSTACK IS EMPTY\t");
			V(semid);
			printf("\n");
			shmdt(stack_pi);
		}
		else if(type==4)
		{
			P(semid);
			double *stack_pi=(double*)shmat(st_shmid,NULL,0);
			int top=pi[(NO_FIELD*id)+1];
			if(top!=-1) {
				printf("\n-------------------DISPLAY------------------\n");
				for(int i=top;i>=0;i--)
					printf(" -> %f",stack_pi[i]);
			}
			else
				printf("\n\tSTACK IS EMPTY\t");
			V(semid);
			printf("\n");
			shmdt(stack_pi);
		}
		else
		{
			printf("No type selected.");
		}			
		*status=1;
		shmdt(pi);
	}	
}

void shstackrm(int shstack_desc_id, int id, int *status) {
	/*this function is to destroy a shared stack*/
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	if(pi[(NO_FIELD*id)+2]>0)	//this check is used to guarantee that a stack can only be destroyed if no other process is using it
	{
		printf("\nThis stack cannot be removed as it is currently used by some process.\nPlease try again later...\n");
		*status=ERRNO;	//setting the status accordingly
		shmdt(pi);
	}
	else
	{
		if(pi[NO_FIELD*id]!=0)	//there exists some stack to destroy
		{
			P(semid);
			shmctl(pi[NO_FIELD*id],0,IPC_RMID);	//destroying the stack
			/*Reinitializing all the fields of the stack descriptor*/
			pi[NO_FIELD*id]=0;
			pi[(NO_FIELD*id)+1]=-1;
			pi[(NO_FIELD*id)+2]=0;
			pi[(NO_FIELD*id)+3]=0;
			pi[(NO_FIELD*id)+4]=0;
			pi[(NO_FIELD*id)+5]=0;
			pi[(NO_FIELD*id)+6]=0;
			printf("\nStack has been successfully destroyed.\n");
			V(semid);
		}
		else 	//no stack exists
			printf("\nNo stack exists to be destroyed.\n");
		*status=1;	//setting the status accordingly
		shmdt(pi);
	}

}

void shstack_busy_on(int shstack_desc_id, int id) {

	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	P(semid);
	pi[(NO_FIELD*id)+2]++;	//incrementing the "no. of processes using this stack" field
	V(semid);
	shmdt(pi);
}

void shstack_busy_off(int shstack_desc_id, int id) {

	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	P(semid);
	if(pi[(NO_FIELD*id)+2]>0)	//checking if the value can be decremented
	pi[(NO_FIELD*id)+2]--;	//decrementing the "no. of processes using this stack" field
	V(semid);
	shmdt(pi);
}

void shstackdescInfo(int shstack_desc_id) {
	/*displaying the stack descriptor*/
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	printf("\n--------------------------------------------------------------------------------------Shared Stack Descriptor Table---------------------------------------------------------------------------\n\n");
	int count=0;
	for(int i=0;i<NO_FIELD*MAX_STACK;i+=NO_FIELD)
	{
		P(semid);
		printf("Id = %d \tShmid = %d%sTop index = %d \tNo of processes using this stack = %d \tData size = %d bytes \tStack size = %d elements \tNo. of elements present = %d \tType = %s\n\n",count++,pi[i],(pi[i]==0?"\t  ":"   "),pi[i+1],pi[i+2],pi[i+3],pi[i+4],pi[i+5],(pi[i+6]==1?"char":pi[i+6]==2?"int":pi[i+6]==3?"float":pi[i+6]==4?"double":"None"));
		V(semid);
	}
	shmdt(pi);

}

int getshstackType(int shstack_desc_id, int id) {
	int type;
	int *pi=(int*)shmat(shstack_desc_id,NULL,0);
	P(semid);
	type=pi[(NO_FIELD*id)+6];	//getting the type of this particular stack
	V(semid);
	return type;
}