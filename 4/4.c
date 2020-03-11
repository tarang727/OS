/*Function to test the shared stacks data structure */
#include "gsharedstack.h"
int main()
{
	int shmid,st_id,status,i,id=-1,choice,des_id,flag,c,type;
	void* value;
	sem_init();

	shmid=shstack_desc_get();
	if(shmid==-1)
	{
		perror("shstack_desc_get() failed: ");
		exit(1);
	}
	printf("Shmid of stack descriptor: %d\n",shmid);
	int *pi=(int*)shmat(shmid,NULL,0);
	
	while(1)
	{
		printf("\n-----------------MENU------------------\n1.Show stack descriptor\n2.Access new stack\n3.Continue operation on current stack\n4.Destroy a stack\n5.Exit\nEnter : ");
		scanf("%d",&choice);
		if(choice==5)
			break;
		switch(choice)
		{
			case 1: shstackdescInfo(shmid);
					break;
			case 2:	printf("Enter the id of stack you want to access (0-9) : ");
					scanf("%d",&id);
					st_id=shstackget(shmid,id);
					flag=0;
					while(1)
					{
						shstack_busy_on(shmid,id);
						int ch,type;
						printf("\n1.Push\n2.Pop\n3.Display\n4.Go back to main menu\nEnter : ");
						scanf("%d",&ch);
						if(ch==4)
						{
							shstack_busy_off(shmid,id);
							break;
						}
						type=getshstackType(shmid,id);
						switch(ch)
						{
							case 1:	if(type==1)
									{
										printf("Enter the char : ");
										char val;
										getchar();
										val=getchar();
										shstackpush(shmid,st_id,id,(void*)&val,&status);
									}
									else if(type==2)
									{
										printf("Enter the int : ");
										int val;
										scanf("%d",&val);
										shstackpush(shmid,st_id,id,(void*)&val,&status);
									}
									else if(type==3)
									{
										printf("Enter float : ");
										float val;
										scanf("%f",&val);
										shstackpush(shmid,st_id,id,(void*)&val,&status);
									}
									else
									{
										printf("Enter the double : ");
										double val;
										scanf("%lf",&val);
										shstackpush(shmid,st_id,id,(void*)&val,&status);										
									}									
									
									if(status==ERRNO)
										flag=1;
									break;
							case 2:	value=shstackpop(shmid,st_id,id,&status);
									if(status==ERRNO)
										flag=1;
									else if(value!=(void*)NULL)
									{
										if(type==1)
										{
											printf("\n%c has been popped from stack.\n", *((char*)value));
										}
										else if(type==2)
										{
											printf("\n%d has been popped from stack.\n", *((int*)value));
										}
										else if(type==3)
										{
											printf("\n%f has been popped from stack.\n", *((float*)value));
										}
										else
										{
											printf("\n%lf has been popped from stack.\n", *((double*)value));	
										}
									
									}
									break;
							case 3: shstackdisplay(shmid,st_id,id,&status);
									if(status==ERRNO)
										flag=1;
									break;
						}
						shstack_busy_off(shmid,id);	
						if(flag==1)
							break;
					}				
					break;
			case 3: if(id!=-1)
					{
						st_id=shstackget(shmid,id);
						flag=0;
					while(1)
					{
						shstack_busy_on(shmid,id);
						int ch,type;
						printf("\n1.Push\n2.Pop\n3.Display\n4.Go back to main menu\nEnter : ");
						scanf("%d",&ch);
						if(ch==4)
						{
							shstack_busy_off(shmid,id);
							break;
						}
						type=getshstackType(shmid,id);
						switch(ch)
						{
							case 1:	if(type==1)
									{
										printf("Enter the char : ");
										char val;
										getchar();
										val=getchar();
										shstackpush(shmid,st_id,id,(void*)&val,&status);
									}
									else if(type==2)
									{
										printf("Enter the int : ");
										int val;
										scanf("%d",&val);
										shstackpush(shmid,st_id,id,(void*)&val,&status);
									}
									else if(type==3)
									{
										printf("Enter float : ");
										float val;
										scanf("%f",&val);
										shstackpush(shmid,st_id,id,(void*)&val,&status);
									}
									else
									{
										printf("Enter the double : ");
										double val;
										scanf("%lf",&val);
										shstackpush(shmid,st_id,id,(void*)&val,&status);										
									}									
									
									if(status==ERRNO)
										flag=1;
									break;
							case 2:	value=shstackpop(shmid,st_id,id,&status);
									if(status==ERRNO)
										flag=1;
									else if(value!=(void*)NULL)
									{
										if(type==1)
										{
											printf("\n%c has been popped from stack.\n", *((char*)value));
										}
										else if(type==2)
										{
											printf("\n%d has been popped from stack.\n", *((int*)value));
										}
										else if(type==3)
										{
											printf("\n%f has been popped from stack.\n", *((float*)value));
										}
										else
										{
											printf("\n%lf has been popped from stack.\n", *((double*)value));	
										}
									
									}
									break;
							case 3: shstackdisplay(shmid,st_id,id,&status);
									if(status==ERRNO)
										flag=1;
									break;
						}
						shstack_busy_off(shmid,id);	
						if(flag==1)
							break;
						}
					}
					else
					{
						printf("\nNo stack chosen yet. Press 2 to first access some stack.\n");
					}
					break;
			case 4: printf("\nEnter the id of the stack you want to destroy : ");
					scanf("%d",&des_id);
					shstackrm(shmid,des_id,&status);
					break;

		}
	}

	shmdt(pi);
//	semctl(semid, 0, IPC_RMID);

	return 0;
}