// Jakub Szpunar cs5460 HW3 Problem 1: Single thread bakery algorithm.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

// Bakery shared arrays.
volatile int* ticket;
volatile int* choosing;
// Used to make sure only one thread is in the critical section at a time.
volatile int in_cs;
// User defined parameters.
int threadCount;
int runTime;
// The time when the program starts to run.
time_t startTime;

void mfence (void) {
  asm volatile ("mfence" : : : "memory");
}

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
	  for(;;)
	    {
	      mfence();
	      if(choosing[i] != 1)
		break;
	    }
	  // Wait until our ticket comes up. Break ties based on threadID.
	  for(;;)
	    {
	      mfence();
	      if(ticket[i] == 0)
	  	break;
	      if(ticket[tid] > ticket[i])
	  	continue;
	      if(ticket[tid] == ticket[i])
		{ 
		  if(tid > i)
		    {
		      continue;
		    }
		}
	      break;
	    }
	}
    }
}

// Release the lock.
void unlock(int tid)
{
  ticket[tid] = 0;
}

// Code for individual threads to run. Try to obtain the lock, make sure it is ours,
// and release the lock. (Looping until time runs out.)
void *thread(void *data)
{
  // Get our thread id.
  int tid = (intptr_t) data;
  // How many times we have been in the critical section.
  int timesInCrit = 0;
  // Store the current time.
  time_t nowTime;
  
  // Loop while we are not out of time.
  do
  {
    time(&nowTime);
    // Get the lock.
    lock(tid);
    // Assert the lock is ours.
    assert (in_cs==0);
    in_cs++;
    assert (in_cs==1);
    in_cs++;
    assert (in_cs==2);
    in_cs++;
    assert (in_cs==3);
    in_cs=0;
    // Release the lock.
    unlock(tid);
    // Incrememebt our times in critical section.
    timesInCrit++;
  }
  while(difftime(nowTime, startTime) < runTime);

  // Return timesInCrit.
  return (void*)(intptr_t)timesInCrit;
}

// Entry point into program. Parses user input, then create
// threads that test the bakery lock.
int main(int argc, char* argv[])
{ 
  // Make sure we have the right number of arguments.
  if(argc != 3)
    {
      printf("Specify thread count and time.\n");
      return 1;
    }

  // Get the arguments and validate.
  threadCount = atoi(argv[1]);
  runTime = atoi(argv[2]);
  if(threadCount < 1 || threadCount > 99)
    {
      printf("ThreadCount must be between 1 and 99.\n");
      return 1;
    }
  if(runTime < 1)
    {
      printf("Runtime must be > 0\n");
      return 1;
    }

  // Malloc bakery arrays and make sure no error occured.
  ticket = (int*)malloc(sizeof(int)*threadCount);
  choosing = (int*)malloc(sizeof(int)*threadCount);
  if(ticket == NULL || choosing == NULL)
    {
      printf("Malloc call failed.\n");
      return 1;
    }
  // Initialize the arrays to zero.
  int i;
  for(i = 0; i < threadCount; i++)
    {
      ticket[i] = 0;
      choosing[i] = 0;
    }

  // Initialize in_cs, get the startTime, and set up an array of pthreads.
  in_cs = 0;
  time(&startTime);
  pthread_t threads[threadCount];

  // Print that we are starting.
  printf("ThreadCount: %i, Runtime: %is.\nRunning...\n", threadCount, runTime);

  // Create threadCount threads and error check.
  for(i = 0; i < threadCount; i++)
    {
      if(pthread_create(&threads[i], NULL, thread, (void*)(intptr_t) i))
	{
	  printf("Error creating thread.\n");
	  return 1;
	}
    }

  // Join threadCount threads and error check.
  for(i = 0; i < threadCount; i++)
    {
      void *status;
      if(pthread_join(threads[i], &status))
	{
	  printf("Error joining thread.\n");
	  return 1;
	}
      // Print how many times we were in the critical section.
      printf("Thread %i:\t%li\n", i, (intptr_t)status);
    }

  return 0;
}


