/*Function to test the shared stacks data structure */
#include "sharedstack.h"
int main()
{
	int shmid,st_id,status,i,id=-1,choice,des_id,flag,c;
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
		printf("\n-----------------MENU------------------\n1.Access new stack\n2.Continue operation on current stack\n3.Destroy a stack\n4.Show stack descriptor\n5.Exit\nEnter : ");
		scanf("%d",&choice);
		if(choice==5)
			break;
		switch(choice)
		{
			case 1:	printf("Enter the id of stack you want to access (0-9) : ");
					scanf("%d",&id);
					st_id=shstackget(shmid,id);
					flag=0;
					while(1)
					{
						shstack_busy_on(shmid,id);
						int ch,val;
						printf("\n1.Push\n2.Pop\n3.Display\n4.Go back to main menu\nEnter : ");
						scanf("%d",&ch);
						if(ch==4)
						{
							shstack_busy_off(shmid,id);
							break;
						}
						switch(ch)
						{
							case 1:	printf("Enter value : ");
									scanf("%d",&val);
									shstackpush(shmid,st_id,id,val,&status);
									if(status==ERRNO)
										flag=1;
									break;
							case 2:	val=shstackpop(shmid,st_id,id,&status);
									if(status==ERRNO)
										flag=1;
									else if(val!=INT_MAX)
										printf("\n%d has been popped from stack.\n", val);
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
			case 2: if(id!=-1)
					{
						st_id=shstackget(shmid,id);
						flag=0;
						while(1)
						{
							shstack_busy_on(shmid,id);
							int ch,val;
							printf("\n1.Push\n2.Pop\n3.Display\n4.Go back to main menu\nEnter : ");
							scanf("%d",&ch);
							if(ch==4)
							{
								shstack_busy_off(shmid,id);	
								break;
							}
							switch(ch)
							{
								case 1:	printf("Enter value : ");
										scanf("%d",&val);
										shstackpush(shmid,st_id,id,val,&status);
										if(status==ERRNO)
											flag=1;
										break;
								case 2:	val=shstackpop(shmid,st_id,id,&status);
										if(status==ERRNO)
											flag=1;
										else if(val!=INT_MAX)
											printf("\n%d has been popped from stack.\n", val);
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
						printf("\nNo stack chosen yet. Press 1 to first access some stack.\n");
					}
					break;
			case 3: printf("\nEnter the id of the stack you want to destroy : ");
					scanf("%d",&des_id);
					shstackrm(shmid,des_id,&status);
					break;
			case 4: shstackdescInfo(shmid);
					break;

		}
	}

	shmdt(pi);
//	semctl(semid, 0, IPC_RMID);

	return 0;
}
