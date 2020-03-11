#include <stdio.h>
#include <stdlib.h> /*exit()*/
#include <sys/wait.h>  /* wait(2) */
#include <unistd.h>  /* fork(2) */
#include <sys/ipc.h>  /* shmget(2), shmctl(2) */
#include <sys/shm.h>  /* shmget(2), shmat(2), shmctl(2) */
#include <sys/types.h> /* shmat(2) shmdt(2)*/
#include <signal.h>  /* catch keybord interupt */
#include <errno.h>	/*perror()*/
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */

#define MAX_STACK 10
#define MAXSIZE 10
#define NO_FIELD 2
#define public_path "/home/tarang/Desktop/os/4/global.txt"
#define public_proj_id 999


int shstack_desc_get()
{
	int shmid;
	key_t public_key=ftok(public_path,public_proj_id);
	shmid=shmget(public_key, NO_FIELD*MAX_STACK*sizeof(int),IPC_EXCL| IPC_CREAT | 0777);

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
		}
		shmdt(pi);
	} 

	return(shmid);
}

int pop(int* stack_pi, int* pi, int id) {

    int data=-1;
    int top=pi[(NO_FIELD*id)+1];
	
   if(top!=-1) {
      data = stack_pi[top];
      top--;  
      
   } else {
      printf("Could not retrieve data, Stack is empty.\n");
   }
   pi[(NO_FIELD*id)+1]=top;
   return data;
}

void push(int* stack_id,int data, int* pi, int id) {

	int top=pi[(NO_FIELD*id)+1];

   if(top!=(MAXSIZE-1)) {
      top++;   
      stack_id[top] = data;
   } else {
      printf("Could not insert data, Stack is full.\n");
   }
   pi[(NO_FIELD*id)+1]=top;
}

void display(int* stack_id, int* pi, int id)
{
	int i;
	int top=pi[(NO_FIELD*id)+1];
	printf("\n-----------DISPLAY-----------\n");
	for(i=top;i>=0;i--)
		printf(" -> %d",stack_id[i]);
	printf("\n");	
}

int main()
{
	int shmid,i,id=-1,choice,des_id,flag,c;
	shmid=shstack_desc_get();
	if(shmid==-1)
	{
		perror("shstack_desc_get() failed: ");
		exit(1);
	}
	printf("shmid: %d\n",shmid);
	int *pi=(int*)shmat(shmid,NULL,0);
	
	printf("Value of stack_desc before:\n");
	for(i=0;i<NO_FIELD*MAX_STACK;i+=NO_FIELD)
	{
		printf("key = %d top = %d\n",pi[i],pi[i+1]);
	}
	while(1)
	{
		printf("\n------------MENU------------\n1.Access new stack\n2.Continue operation on current stack\n3.Destroy a stack\n4.Exit\nEnter : ");
		scanf("%d",&choice);
		if(choice==4)
			break;
		switch(choice)
		{
			case 1:	printf("Enter the id of stack you want to access : ");
					scanf("%d",&id);
					if(pi[NO_FIELD * id]==0)
					{
						printf("No such stack exists of now.\n");
						key_t st_key=ftok(public_path, (id+1));
						int st_id=shmget(st_key,MAXSIZE*sizeof(int),IPC_CREAT|0777);
						printf("New stack created with shmid : %d\n",st_id);
						pi[NO_FIELD * id]=st_id;
						int *st_pi=(int*)shmat(st_id,NULL,0);
						flag=0;
						while(1)
						{
							int ch,val;
							printf("\n1.Push\n2.Pop\n3.Display\n4.Exit\nEnter : ");
							scanf("%d",&ch);
							if(ch==4)
								break;
							switch(ch)
							{
								case 1:	printf("Enter value : ");
										scanf("%d",&val);
										if(pi[NO_FIELD*id]!=0)
											push(st_pi,val,pi,id);
										else
										{
											printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
											flag=1;
										}
										break;
								case 2:	if(pi[NO_FIELD*id]!=0)
										{
											val=pop(st_pi,pi,id);
											if(val!=-1)
											printf("Value popped = %d\n",val);
										}
										else
										{
											printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
											flag=1;
										}
										break;
								case 3: if(pi[NO_FIELD*id]!=0)
											display(st_pi,pi,id);
										else
										{
											printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
											flag=1;
										}
										break;
							}
							if(flag==1)
								break;
						}

						shmdt(st_pi);
					//	shmctl(st_id,0,IPC_RMID);
					}
					else
					{
						int st_id=pi[NO_FIELD * id];
						int *st_pi=(int*)shmat(st_id,NULL,0);
						flag=0;
						while(1)
						{
							int ch,val;
							printf("\n1.Push\n2.Pop\n3.Display\n4.Exit\nEnter : ");
							scanf("%d",&ch);
							if(ch==4)
								break;
							switch(ch)
							{
								case 1:	printf("Enter value : ");
										scanf("%d",&val);
										if(pi[NO_FIELD*id]!=0)
											push(st_pi,val,pi,id);
										else
										{
											printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
											flag=1;
										}
										break;
								case 2:	if(pi[NO_FIELD*id]!=0)
										{
											val=pop(st_pi,pi,id);
											if(val!=-1)
											printf("Value popped = %d\n",val);
										}
										else
										{
											printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
											flag=1;
										}
										break;
								case 3: if(pi[NO_FIELD*id]!=0)
											display(st_pi,pi,id);
										else
										{
											printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
											flag=1;
										}
										break;
							}
							if(flag==1)
								break;
						}

						shmdt(st_pi);
					}
					break;
			case 2: if(id!=-1)
					{
						if(pi[NO_FIELD * id]==0)
						{
							printf("No such stack exists of now.\n");
							key_t st_key=ftok(public_path, (id+1));
							int st_id=shmget(st_key,MAXSIZE*sizeof(int),IPC_CREAT|0777);
							printf("New stack created with id : %d\n",st_id);
							pi[NO_FIELD * id]=st_id;
							int *st_pi=(int*)shmat(st_id,NULL,0);
							flag=0;
							while(1)
							{
								int ch,val;
								printf("\n1.Push\n2.Pop\n3.Display\n4.Exit\nEnter : ");
								scanf("%d",&ch);
								if(ch==4)
									break;
								switch(ch)
								{
									case 1:	printf("Enter value : ");
											scanf("%d",&val);
											if(pi[NO_FIELD*id]!=0)
												push(st_pi,val,pi,id);
											else
											{
												printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
												flag=1;
											}
											break;
									case 2:	if(pi[NO_FIELD*id]!=0)
											{
												val=pop(st_pi,pi,id);
												if(val!=-1)
												printf("Value popped = %d\n",val);
											}
											else
											{
												printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
												flag=1;
											}
											break;
									case 3: if(pi[NO_FIELD*id]!=0)
												display(st_pi,pi,id);
											else
											{
												printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
												flag=1;
											}
											break;
								}
								if(flag==1)
									break;
							}

							shmdt(st_pi);
						//	shmctl(st_id,0,IPC_RMID);
						}
						else
						{
							int st_id=pi[NO_FIELD * id];
							int *st_pi=(int*)shmat(st_id,NULL,0);
							flag=0;
							while(1)
							{
								int ch,val;
								printf("\n1.Push\n2.Pop\n3.Display\n4.Exit\nEnter : ");
								scanf("%d",&ch);
								if(ch==4)
									break;
								switch(ch)
								{
									case 1:	printf("Enter value : ");
											scanf("%d",&val);
											if(pi[NO_FIELD*id]!=0)
												push(st_pi,val,pi,id);
											else
											{
												printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
												flag=1;
											}
											break;
									case 2:	if(pi[NO_FIELD*id]!=0)
											{
												val=pop(st_pi,pi,id);
												if(val!=-1)
												printf("Value popped = %d\n",val);
											}
											else
											{
												printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
												flag=1;
											}
											break;
									case 3: if(pi[NO_FIELD*id]!=0)
												display(st_pi,pi,id);
											else
											{
												printf("Sorry. Cannot perform operation. Stack has been suddenly destroyed !!\n");
												flag=1;
											}
											break;
								}
								if(flag==1)
									break;
							}


							shmdt(st_pi);
						}
					}
					else
					{
						printf("No stack chosen yet. Press 1 to first access some stack.\n");
					}
					break;
			case 3: printf("\nEnter id of stack you want to destroy : ");
					scanf("%d",&des_id);
					if(pi[NO_FIELD * des_id]!=0)
					{
						printf("This stack is presently being used by some process. Are you sure you want to remove the stack ?\nPress 1 for yes and 0 for no\n");
						scanf("%d",&c);
						if(c==1)
						{
							shmctl(pi[NO_FIELD*des_id],0,IPC_RMID);
							pi[NO_FIELD*des_id]=0;
							pi[(NO_FIELD*des_id)+1]=-1;
							printf("Stack has been successfully destroyed.\n");

						}
						else
							break;

					}
					else
						printf("No stack exists to be destroyed.\n");
					break;

		}
	}

	printf("Value of stack_desc after :\n");
	for(i=0;i<NO_FIELD*MAX_STACK;i+=NO_FIELD)
	{
		printf("key = %d top = %d\n",pi[i],pi[i+1]);
	}
	shmdt(pi);
//	shmctl(shmid,0,IPC_RMID);

	return 0;
}
