#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/

#define MAX_STACK 5
#define MAXSIZE 10

typedef struct
{
	int id;
	int* stack;
	int top;
} stackdesc;

stackdesc shared_stacks[MAX_STACK];

struct passedData{
	int thread_id;
	int stack_id;
};

struct passedData *threadData;

int isempty(int stack_id) {

   if(shared_stacks[stack_id].top == -1)
      return 1;
   else
      return 0;
}
   
int isfull(int stack_id) {

   if(shared_stacks[stack_id].top == MAXSIZE)
      return 1;
   else
      return 0;
}


int pop(int stack_id) {
   int data=-1;
	
   if(!isempty(stack_id)) {
      data = shared_stacks[stack_id].stack[(shared_stacks[stack_id].top)];
      (shared_stacks[stack_id].top)--;  
      
   } else {
      printf("Could not retrieve data, Stack is empty.\n");
   }
   return data;
}

void push(int stack_id,int data) {

   if(!isfull(stack_id)) {
      (shared_stacks[stack_id].top)++;   
      shared_stacks[stack_id].stack[(shared_stacks[stack_id].top)] = data;
   } else {
      printf("Could not insert data, Stack is full.\n");
   }
}

void display()
{
	int i,j;
	printf("\n-----------DISPLAY-----------\n");
	for(i=0;i<MAX_STACK;i++)
	{
		printf("Stack number %d :\n",i);
		for(j=shared_stacks[i].top;j>=0;j--)
			printf("%d -> ",shared_stacks[i].stack[j]);
		printf("\n");
	}
}


void *executor(void *param)
{
	struct passedData *data=(struct passedData*)param;
	int r,val;
	r=rand()%50;
	if(r%2==0)
	{
		push(data->stack_id,r);
		printf("Value pushed by thread %d in stack %d is: %d\n",data->thread_id,data->stack_id,r);
		display();

	}
	else
	{
		val=pop(data->stack_id);
		printf("Value popped by thread %d from stack %d is: %d\n",data->thread_id,data->stack_id,val);
		display();
	}

	free(param);
	pthread_exit(0);

}

pthread_mutex_t st_mutex;

int main()
{
	int i,n,r,status;
	printf("How many threads do you want to create ?\n");
	scanf("%d",&n);
	pthread_t tids[n];
	pthread_attr_t attr;
	status = pthread_mutex_init(&st_mutex, NULL);
	pthread_attr_init(&attr); 

	srand(time(NULL));
	for(i=0;i<MAX_STACK;i++)
	{
		shared_stacks[i].id=i;
		shared_stacks[i].stack=(int*)calloc(MAXSIZE,sizeof(int));
		shared_stacks[i].top=-1;

	}

	for(i=0;i<n;i++)
	{
		r = rand()%MAX_STACK;
		threadData = (struct passedData *)malloc(sizeof(struct passedData));
		threadData->stack_id=r;
		threadData->thread_id=i;
		status = pthread_create(&tids[i], &attr, executor, threadData);
		if (status != 0)
	    { 
	        fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
	        exit(1);
	    }

	}


  for(i=0;i<n;i++)
  {
  	pthread_join(tids[i],NULL);
  }

}