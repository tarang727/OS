#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/
#include <math.h>

struct info//data structure to be sent to the thread function
{
  int *array;
  int index;
  int extra;

};



pthread_mutex_t mutex;

void *threadfunction(void *param)//thread function for step by step addition
{

  struct info *data=(struct info *)param;
  int i=data->index;
  int e=data->extra;
  int sum=data->array[i]+data->array[i+(e/2)];
  data->array[i]=sum;
  printf("Thread Sum=%d\n",sum);
  free(param);
  pthread_exit(0);

}

/*thread function for adding the first and last element until it becomes a multiple of 2*/
void *threadfunction1(void *param)
{

  struct info *data=(struct info *)param;
  int i=data->index;
  int n=data->extra;
  data->array[0]=data->array[0]+data->array[n-1];
  free(param);
  pthread_exit(0);

}

/*function that returns the closest power of 2 smaller than or equla to the number*/
int isPowerOfTwo(int n)
{
  int count=0;
    
  while (n != 1)
  {
      count++;
      n = n/2;
  }
  
  return count;

}

int my_log(int x, int base)
{
  return (int)(log(x) / log(base)); 
}


int sumOfElements(int data[] , int size)
{
 int n;
 int i,j;
 int status;
 int count;
 
 
 int actual_size=size;
 int step=2;
 int stage=1;

 pthread_attr_t attr;
 int ret=pthread_attr_init(&attr);

  status = pthread_mutex_init(&mutex, NULL);
  printf("Mutex status = %d",status);
 //printf("Ret=%d",ret);

 int iterator;
// iterator=isPowerOfTwo(size);
  iterator=my_log(size,2);

  pthread_t tid[actual_size]; 
  //fprintf(stdout,"Size is=%d",size);
  while(iterator>0)
   {
     size=size/2;
     pthread_mutex_lock(&mutex);
     for(i=0,j=0;i<actual_size,j<size;i+=step,j++)
     {

        struct info *thread_data=malloc(sizeof(struct info));
        
        thread_data->array=data;//sending the array to the structure
        thread_data->index=i;
        thread_data->extra=step;
        
        status=pthread_create(&tid[j],&attr,threadfunction,thread_data);
        
        if (status != 0)
        { 
          fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
          exit(1);
        }

     }
     
     pthread_mutex_unlock(&mutex); 
     for(i=0;i<size;i++)
     {
      
      status=pthread_join(tid[i],NULL);
      
      if (status != 0) 
      { 
          fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
      }

     } 
     
    printf("After Stage%d\n",stage);
     
     for(i=0;i<actual_size;i++)
         {printf("%d ",data[i]);}//*/
     step*=2;
     //size=size/2;
     stage++;
     //printf("\nSIZE=%d\n",size);
     iterator--;
   }
   pthread_attr_destroy(&attr);
   return data[0];




}



int main()
{
 
 int *array;
 int sum,n,i,size,count;

 printf("Enter size of the array\n");
 scanf("%d",&n);

 array=malloc(n*sizeof(int));

 printf("Enter the array elements\n");

 for(i=0;i<n;i++)
  scanf("%d",&array[i]);

 printf("Enter size of the array on which to perform operation\n");
 scanf("%d",&size);

 //int x=isPowerOfTwo(size);
 int x=my_log(size,2);
 //printf("%d",x);

 pthread_attr_t attr;
 int ret=pthread_attr_init(&attr);
 
 if(pow(2,x)!=size)//adding first and last element until it becomes power of 2
 {
  int k=size-pow(2,x);
  pthread_t tid[k];
  int status;

  for(int i=0;i<k;i++)
   {
        struct info *thread_data=malloc(sizeof(struct info));
        thread_data->array=&array[0];
        thread_data->extra=size;
        status=pthread_create(&tid[i],&attr,threadfunction1,thread_data);
        if (status != 0)
        { 
          fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
          exit(1);
        }  
        size--;
   }
   //printf("\nsize=%d\n",size);

   for(i=0;i<k;i++)
     {
      
       status=pthread_join(tid[i],NULL);
      
      if (status != 0) 
      { 
          fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
      }

     } 

   } 
 
 sum=sumOfElements(array,size);
 printf("The sum is=%d\n",sum);

 return 0;

}