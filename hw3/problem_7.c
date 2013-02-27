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
int threadCount;
int runTime;
time_t startTime;

#ifdef BAKERY

volatile int* ticket;
volatile int* choosing;

// Return the maximum ticket in the ticket array.
int maxTicket()
{
  int maxTicket = 0;
  int i;
  for(i = 0; i < threadCount; i++)
    if(ticket[i] > maxTicket)
      maxTicket = ticket[i];
  return maxTicket;
}

// Obtain the lock.
void lock(int tid)
{
  // Get a ticket.
  choosing[tid] = 1;
  ticket[tid] = maxTicket() + 1;
  choosing[tid] = 0;

  // Wait for the ticket to be up.
  int i;
  for(i = 0; i < threadCount; ++i)
    {
      // Don't wait on yourself.
      if(i != tid)
	{
	  // Wait if another thread is choosing a ticket.
	  while(choosing[i] == 1){}
	  // Wait until our ticket comes up. Break ties based on threadID.
	  while(ticket[i] != 0 &&( ticket[tid] > ticket[i] || (ticket[tid] == ticket[i] && tid > i))) {}
	}
    }
}

// Release the lock.
void unlock(int tid)
{
  ticket[tid] = 0;
}

#endif


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

void *proThread(void* data)
{
  #ifdef BAKERY
  int tid = (intptr_t)data;
  #endif

  int proCount = 0;
  time_t nowTime;
  
  do
    {
      time(&nowTime);
      
#ifdef BAKERY
      lock(tid);
#endif
	
      if(enq(7))
	proCount++;
	
      
#ifdef BAKERY
      unlock(tid);
#endif
	
    }
  while(difftime(nowTime, startTime) < runTime);

  return (void*)(intptr_t)proCount;
}

void *conThread(void* data)
{
  #ifdef BAKERY
  int tid = (intptr_t)data;
  #endif
  int conCount = 0;
  time_t nowTime;
  int res;

  do
    {
      time(&nowTime);
      
      #ifdef BAKERY
      lock(tid);
      #endif

      if(deq(&res))
	conCount++;
       
      
#ifdef BAKERY
      lock(tid);
#endif

    }
  while(difftime(nowTime, startTime) < runTime);
  
  return (void*)(intptr_t)conCount;
}



int main(int argc, char *argv[])
{ 
  int i;
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
  threadCount = proThreadCount + conThreadCount;
  
  // Set up the queue.
  queue = malloc(MAX_QUEUE_SIZE*sizeof(int));
  if(queue == NULL)
    {
      printf("Malloc error.\n");
      return 1;
    }
  initialize();

  #ifdef BAKERY
  // Malloc bakery arrays and make sure no error occured.
  ticket = (int*)malloc(sizeof(int)*(proThreadCount+conThreadCount));
  choosing = (int*)malloc(sizeof(int)*(proThreadCount+conThreadCount));
  if(ticket == NULL || choosing == NULL)
    {
      printf("Malloc call failed.\n");
      return 1;
    }
  // Initialize the arrays to zero.

  for(i = 0; i < threadCount; i++)
    {
      ticket[i] = 0;
      choosing[i] = 0;
    }
  #endif

  // Set up the threads.
  pthread_t proThreads[proThreadCount];
  pthread_t conThreads[conThreadCount];
  time(&startTime);

  printf("Starting up %i producers and %i consumers for %i seconds.\n", proThreadCount, conThreadCount, runTime);


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
      if(pthread_create(&conThreads[i], NULL, conThread, (void*)(intptr_t) (i+proThreadCount)))
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
      printf("Producer %i:\t%li\n", i, (intptr_t)status);
    }

  printf("\n");
  for(i = 0; i < conThreadCount; i++)
    {
      void *status;
      if(pthread_join(conThreads[i], &status))
	{
	  printf("Error joining thread.\n");
	  return 1;
	}
      printf("Consumer %i:\t%li\n", i, (intptr_t)status);
    }

  return 0;
}


