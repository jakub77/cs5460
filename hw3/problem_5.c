// Jakub Szpunar cs5460 HW3 Problem 5. fair spin lock.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

// Used to make sure only one thread is in the critical section at a time.
volatile int in_cs;
// User defined parameters.
int threadCount;
int runTime;
// The time when the program starts to run.
time_t startTime;

// Used to coordinate threads
struct spin_lock_t
{
  // The client being served.
  volatile int serving;
  // The position of the last client.
  volatile int inLine;
};

/* atomic_xadd
 * equivalent to atomic execution of this code:
 * return (*ptr)++;
 */
static inline int atomic_xadd (volatile int *ptr)
{
  register int val __asm__("eax") = 1;
  asm volatile ("lock xaddl %0,%1"
  : "+r" (val)
  : "m" (*ptr)
  : "memory"
  );  
  return val;
}

void spin_lock(struct spin_lock_t *s)
{
  // Get the next line position.
  int pos = atomic_xadd(&(s->inLine));
  // Spin until our position is being served.
  while (pos != s->serving){}
  return;
}

void spin_unlock(struct spin_lock_t *s)
{
  // Incremement serving so the next thread can go.
  atomic_xadd(&(s->serving));
  return;
}

// Code for individual threads to run. Try to obtain the lock, make sure it is ours,
// and release the lock. (Looping until time runs out.)
void *thread(void *data)
{
  // Spin lock data.
  void* sl = (void*)data;
  // How many times we have been in the critical section.
  int timesInCrit = 0;
  // Store the current time.
  time_t nowTime;
  
  // Loop while we are not out of time.
  do
    {
      time(&nowTime);
      // Get the lock.
      spin_lock(sl);
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
      spin_unlock(sl);
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

  // Initialize in_cs, get the startTime, and set up an array of pthreads.
  in_cs = 0;
  time(&startTime);
  pthread_t threads[threadCount];

  // Create a spinlock object
  struct spin_lock_t sl = {0, 0};
  
  // Print that we are starting.
  printf("ThreadCount: %i, Runtime: %is.\nRunning...\n", threadCount, runTime);

  // Create threadCount threads and error check.
  int i;
  for(i = 0; i < threadCount; i++)
    {
      if(pthread_create(&threads[i], NULL, thread, (void*)&sl ))
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
