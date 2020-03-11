#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/
#include <math.h> //to find log

struct passedData{
	int index;	//to store first index of the 2 nos to be added
	int stage;	//to store the value to be added to index value at each stage which finally gives second number
	int* data;	//the array on which operations are performed
};


pthread_mutex_t st_mutex;	//defining the mutex

int my_log(int x, int base)	//function to find log x base 2
{
  return (int)(log(x) / log(base)); 
}

void *executor1(void *param)	//this function is invoked to find sum of first and last element till the size is a power 2
{
	int n,sum;
	struct passedData *recvd=(struct passedData*)param;	
	n=recvd->stage;	//receiving the value of n through stage
	sum=recvd->data[0]+recvd->data[n-1]; //performing the addition
	recvd->data[0]=sum;	//storing added value in array[0]
	free(param);	//freeing the memory
	pthread_exit(0);

}

void *executor2(void *param)	//this function performs additions of 2 elements as specified in question
{
	int i,stage,sum;
	struct passedData *recvd=(struct passedData*)param;
	i=recvd->index;
	stage=recvd->stage;
	sum=recvd->data[i]+recvd->data[i+(stage/2)]; //performing the addition
	recvd->data[i]=sum;
	free(param);	
	pthread_exit(0);

}


int sumofElements(int data[],int size)
{
	int i,j,n,c=1,step=2,status,counter,r;
	
	pthread_attr_t attr;
	pthread_attr_init(&attr); //initializing the attributes
	status = pthread_mutex_init(&st_mutex, NULL);	//initializing the mutex
	if(status!=0)
	{
		fprintf(stderr, "pthread_mutex_init() failed: %s.\n", status == EAGAIN?"The system lacked the necessary resources (other than memory) to initialize another mutex.":status == ENOMEM?"Insufficient memory exists to initialize the mutex.":status == EPERM?"The caller does not have the privilege to perform the operation.":status==EINVAL?"The attributes object referenced by attr has the robust mutex attribute set without the process-shared attribute being set.":"Unknown Error");
		exit(1);
	}
//	printf("Mutex status = %d",status);
	
	int limit = my_log(size,2); //finding log(size) base 2
	pthread_t tids[size];

//If size is not a power of 2
	if(pow(2,limit)!=size)
	{
		//remainder size for which sum of 1st and last element needs to be found
		r=size-pow(2,limit);

		for(i=0;i<r;i++)
		{
			struct passedData *threadData = (struct passedData *)malloc(sizeof(struct passedData));
			threadData->stage=size;	//passing the size
			threadData->data=data;	//passing the array
			status = pthread_create(&tids[i], &attr, executor1, threadData);	//pointer to executor1 function is passed, so it will get executed
			if (status != 0)
		    { 
		        fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
		        exit(1);
		    }

			size--;
		}



	   for(i=0;i<r;i++)	//joining all the threads
	     {
	      
	       status=pthread_join(tids[i],NULL);
	      
	      if (status != 0) 
	      { 
	          fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
	      }

	     } 


	}
	n=size;
	while(limit>0)
	{
		n=n/2;	//everytime no of threads created gets halved
		status =  pthread_mutex_lock(&st_mutex);	//In single stage only those threads should be executed
		//	printf("Mutex status = %d",status);
		counter=0;
		for(i=0;i<size;i+=step)	//step=2, 4, 8... in each stage
		{
			
			struct passedData *threadData = (struct passedData *)malloc(sizeof(struct passedData));
			threadData->stage=step;
			threadData->index=i;
			threadData->data=data;
			status = pthread_create(&tids[i], &attr, executor2, threadData);	//pointer to executor2 function is passed, so it will get executed
			if (status != 0)
		    { 
		        fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
		        exit(1);
		    }

		}
		pthread_mutex_unlock(&st_mutex);
		for(i=0;i<size;i+=step)
		{
	      status=pthread_join(tids[i],NULL);	//all created threads are to be joined 
	      
	      if (status != 0) 
	      { 
	          fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
	      }
		}

		 printf("Stage %d:\n",c);
	     
	     for(i=0;i<size;i++)
	     {
	         printf("%d ",data[i]);
	     }
	     printf("\n");
		step*=2;
		c++;	//index for tids[]
		limit--;
	

	}

	return data[0];
}

int main()
{
 
	 int i,n,size,sum;

	 printf("Enter total size: \n");
	 scanf("%d",&n);

	 int* data;
	 data=malloc(n*sizeof(int));

	 printf("Enter the elements:\n");

	 for(i=0;i<n;i++)
	 	scanf("%d",&data[i]);

	 printf("Enter size to perform addition: \n");
	 scanf("%d",&size);

	 sum=sumofElements(data,size);
	 printf("The final sum is=%d\n",sum);

	 printf("The elements of array are :\n");
	 for(i=0;i<n;i++)
	 	printf("%d ",data[i]);

	 printf("\n");

	 return 0;


}
