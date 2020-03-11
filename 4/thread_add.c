#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/
//#include <math.h>

#define MAXSIZE 10

int univ_data[MAXSIZE];



struct passedData{
	int stage;
	int index;
	int size;
	int* data;
};

struct passedData *threadData;

pthread_mutex_t st_mutex;


void *threadfunction(void *param)
{

  struct info *data=(struct info *)param;
  int i=data->index;
  int sum=data->array[2*i]+data->array[2*i+1];
  data->array[2*i]=sum;
  printf("Thread SUm=%d\n",sum);
  free(param);
  pthread_exit(0);

}

void *executor(void *param)
{
	struct passedData *recvd=(struct passedData*)param;
	int i,sum;

	i=recvd->index;
	sum=recvd->data[2*i]+recvd->data[2*i+1];
	recvd->data[2*i]=sum;
	free(param);
	pthread_exit(0);

}

int sumofElements(int data[MAXSIZE],int size)
{
	int i,j,n,c=-1,step=1;
	
	pthread_attr_t attr;
	status = pthread_mutex_init(&st_mutex, NULL);
	pthread_attr_init(&attr); 
	n=size;

	if(size%2==0)
	{
		while(n>=2)
		{
			pthread_t tids[n];
			for(i=0;i<n/2;i+=step)
			{
				threadData = (struct passedData *)malloc(sizeof(struct passedData));
				threadData->stage=c;
				threadData->index=i;
				threadData->size=size;
				thread_data->array=&data[0];
				status = pthread_create(&tids[i], &attr, executor, threadData);
				if (status != 0)
			    { 
			        fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
			        exit(1);
			    }

			}
			for(i=0;i<n;i+=step)
			{
				pthread_join(tids[i],NULL);
			}

			step*=2;
			size/=2;

		}

		return data[0];
	}



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
	 	scanf("%d",&array[i]);

	 printf("Enter size to perform addition: \n");
	 scanf("%d",&size);

	 sum=sumofElements(data,size);
	 printf("The sum is=%d\n",sum);

	 printf("The elements of array are :\n");
	 for(i=0;i<n;i++)
	 	printf("%d",data[i]);

	 return 0;


}
