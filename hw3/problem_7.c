// Jakub Szpunar cs5460 HW3 Problem 7: Single thread bakery algorithm.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

// pointer to the queue.
int* queue;
static const int MAX_QUEUE_SIZE = 32;
int enqPos;
int deqPos;
int curQueueSize;
int proThreadCount;
int conThreadCount;
int runTime;


int enq(int value)
{
  if(curQueueSize >= 32)
    return 0;
  
  queue[enqPos] = value;

  if(enqPos == MAX_QUEUE_SIZE - 1)
    enqPos = 0;
  else
    enqPos++;

  curQueueSize++;
  return 1;
}

int deq(int* value)
{
  if(curQueueSize == 0)
    return 0;

  (*value) = queue[deqPos];

  if(deqPos == MAX_QUEUE_SIZE - 1)
    deqPos = 0;
  else
    deqPos++;

  curQueueSize--;
  return 1;
}

void initialize()
{
  enqPos = 0;
  deqPos = 0;
  curQueueSize = 0;
  return;
}

void *proThread(void *data)
{
  return data;
}

void *conThread(void *data)
{
  return data;
}



int main(int argc, char *argv[])
{ 
  // Make sure we have the right number of arguments.
  if(argc != 4)
    {
      printf("Specify producer count, consumer count, and time.\n");
      return 1;
    }

  // Get the arguments and validate.
  proThreadCount = atoi(argv[1]);
  conThreadCount = atoi(argv[2]);
  runTime = atoi(argv[3]);
  if(proThreadCount < 1 || proThreadCount > 99)
    {
      printf("Producer thread count must be between 1 and 99.\n");
      return 1;
    }
  if(conThreadCount < 1 || conThreadCount > 99)
    {
      printf("Consumer thread count must be between 1 and 99.\n");
      return 1;
    }
  if(runTime < 1)
    {
      printf("Runtime must be > 0\n");
      return 1;
    }
  
  // Set up the queue.
  queue = malloc(MAX_QUEUE_SIZE*sizeof(int));
  if(queue == NULL)
    {
      printf("Malloc error.\n");
      return 1;
    }
  initialize();

  // Set up the threads.
  pthread_t proThreads[proThreadCount];
  pthread_t conThreads[conThreadCount];

  printf("Starting up %i producers and %i consumers for %i seconds.\n", proThreadCount, conThreadCount, runTime);

  int i;
  for(i = 0; i < proThreadCount; i++)
    {
      if(pthread_create(&proThreads[i], NULL, proThread, (void*)(intptr_t) i))
	{
	  printf("Error creating thread.\n");
	  return 1;
	}
    }
  for(i = 0; i < conThreadCount; i++)
    {
      if(pthread_create(&conThreads[i], NULL, conThread, (void*)(intptr_t) i))
	{
	  printf("Error creating thread.\n");
	  return 1;
	}
    }

  for(i = 0; i < proThreadCount; i++)
    {
      void *status;
      if(pthread_join(proThreads[i], &status))
	{
	  printf("Error joining thread.\n");
	  return 1;
	}
    }

  for(i = 0; i < conThreadCount; i++)
    {
      void *status;
      if(pthread_join(conThreads[i], &status))
	{
	  printf("Error joining thread.\n");
	  return 1;
	}
    }

  return 0;
}


