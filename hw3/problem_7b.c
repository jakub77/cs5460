// Jakub Szpunar cs5460 HW3 Problem 7: Locked Queue Test
// Note lock implementations are not commented to save space, to see
// lock comments, go to the specific problem that deals with that lock.
//
// To compile code a -D must be sent.
// use -DBAKERY for bakery, -DSPINLOCK for spinlock, -DFAIRSPINLOCK for the
// fair spin lock, and -DMUTEX for a pthreads mutex. Defining two things at
// once will not work/compile.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <math.h>

// pointer to the queue.
int* queue;
static const int MAX_QUEUE_SIZE = 32;
// The positions of the queue the implementaiton is at.
volatile int enqPos;
volatile int deqPos;
volatile int curQueueSize;
// The thread counts.
int proThreadCount;
int conThreadCount;
int threadCount;
// The timer code.
int runTime;
time_t startTime;

// Error testing code


// Conditional compilation of different locks.
#ifdef BAKERY
volatile int* ticket;
volatile int* choosing;
void mfence (void) {
  asm volatile ("mfence" : : : "memory");
}
int maxTicket()
{
  int maxTicket = 0;
  int i;
  for(i = 0; i < threadCount; i++)
    if(ticket[i] > maxTicket)
      {
	maxTicket = ticket[i];
      }
  return maxTicket;
}
void lock(int tid)
{
  choosing[tid] = 1;
  mfence();
  ticket[tid] = maxTicket() + 1;
  choosing[tid] = 0;
  int i;
  for(i = 0; i < threadCount; ++i)
    {
      if(i != tid)
	{
	  for(;;)
	    {
	      mfence();
	      if(choosing[i] != 1)
		break;
	    }
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
void unlock(int tid)
{
  ticket[tid] = 0;
}
#endif

#ifdef SPINLOCK
struct spin_lock_t
{
  volatile int lock;
};
volatile struct spin_lock_t *sl;
static inline int atomic_cmpxchg (volatile int *ptr, int old, int new)
{
  int ret;
  asm volatile ("lock cmpxchgl %2,%1"
    : "=a" (ret), "+m" (*ptr)     
    : "r" (new), "0" (old)      
    : "memory");         
  return ret;                            
}
void spin_lock(struct spin_lock_t *s)
{
  while(atomic_cmpxchg(&(s->lock), 0, 1)){}
  return;
}

void spin_unlock(struct spin_lock_t *s)
{
  s->lock = 0;
  return;
}
#endif

#ifdef FAIRSPINLOCK
struct spin_lock_t
{
  volatile int serving;
  volatile int inLine;
};
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
  int pos = atomic_xadd(&(s->inLine));
  while (pos != s->serving){}
  return;
}

void spin_unlock(struct spin_lock_t *s)
{
  atomic_xadd(&(s->serving));
  return;
}
#endif

#ifdef MUTEX
pthread_mutex_t lock;
#endif

// Function to calculate the standard deviation of an array of data.
float std_dev(int data[], int n)
{
  float mean = 0.0, sum_deviation = 0.0;
  int i;
  for(i = 0; i < n; i++)
    mean += data[i];
  mean /= n;
  for(i = 0; i < n; i++)
    sum_deviation += (data[i]-mean)*(data[i]-mean);
  return sqrt(sum_deviation/n);
}

// Enqueue something into the queue.
// Returns 1 if success, 0 if queue was full.
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

// Dequeue something from the queue.
// Return 1 if success, 0 if queue empty, 
// *value is set to the dequeued value if success.
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

// Initliaze the queue, also initialize lock code if possible.
// Return 0 if an error occured, returns 1 otherwise.
int initialize()
{
  // Queue stuff.
  enqPos = 0;
  deqPos = 0;
  curQueueSize = 0;

  // Lock stuff
  #ifdef BAKERY
  ticket = (int*)malloc(sizeof(int)*threadCount);
  choosing = (int*)malloc(sizeof(int)*threadCount);
  if(ticket == NULL || choosing == NULL)
    {
      printf("Malloc call failed.\n");
      return 0;
    }
  int i;
  for(i = 0; i < threadCount; i++)
    {
      ticket[i] = 0;
      choosing[i] = 0;
    }
  #endif

  #ifdef MUTEX
  if(pthread_mutex_init(&lock, NULL))
    {
      printf("Mutex init failed\n");
      return 0;
    }
  #endif
  return 1;
}

// A producer thread. Returns the number of times it was able to
// add to the queue. 
#ifdef MUTEX
void *proThread()
#else
void *proThread(void *data)
#endif
{
  // Set up the count/time.
  int proCount = 0;
  time_t nowTime;

  // Cast the argument depending on what lock.
  #ifdef BAKERY
  int tid = (intptr_t)data;
  #endif
  #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
  void* sl = (void*)data;
  #endif

  // Loop locking and adding to the queue.
  do
    {
      // Lock using the selected lock.
      time(&nowTime);
      #ifdef BAKERY
      lock(tid);
      #endif
      #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
      spin_lock(sl);
      #endif
      #ifdef MUTEX
      pthread_mutex_lock(&lock);
      #endif
      
      // Try to add to the queue, if success, incremement count.
      if(enq(7))
	proCount++;

      // Unlock the selected lock.
      #ifdef BAKERY
      unlock(tid);
      #endif
      #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
      spin_unlock(sl);
      #endif
      #ifdef MUTEX
      pthread_mutex_unlock(&lock);
      #endif
    }
  while(difftime(nowTime, startTime) < runTime);

  // Return the count.
  return (void*)(intptr_t)proCount;
}

// A consumer thread tries to remove from the queue as fast as possible.
// Returns the number of consumptions.
#ifdef MUTEX
void *conThread()
#else
void *conThread(void *data)
#endif
{
  // Set up count, time and the result of a dequeue.
  int conCount = 0;
  time_t nowTime;
  int res;

  // Cast the arguments depending on the lock.
  #ifdef BAKERY
  int tid = (intptr_t)data;
  #endif
  #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
  void* sl = (void*)data;
  #endif

  // Loop locking, dequeueing, and unlocking.
  do
    {
      // Lock the selected lock.
      time(&nowTime);
      #ifdef BAKERY
      lock(tid);
      #endif
      #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
      spin_lock(sl);
      #endif
      #ifdef MUTEX
      pthread_mutex_lock(&lock);
      #endif

      // Dequeue, if success, incremement count.
      if(deq(&res))
	conCount++;

      // Unlock the selected lock.
      #ifdef BAKERY
      unlock(tid);
      #endif
      #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
      spin_unlock(sl);
      #endif
      #ifdef MUTEX
      pthread_mutex_unlock(&lock);
      #endif
    }
  while(difftime(nowTime, startTime) < runTime);

  // Return the number of dequeues. 
  return (void*)(intptr_t)conCount;
}

// Entry point into the program. Parses arguments, then creates
// threads to enqueue and dequeue as fast as possible.
// Outputs statistics about the operations at the end.
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
  // Set the toal thread count.
  threadCount = proThreadCount + conThreadCount;
  
  // Set up the queue.
  queue = malloc(MAX_QUEUE_SIZE*sizeof(int));
  if(queue == NULL)
    {
      printf("Malloc error.\n");
      return 1;
    }
  
  // Initialize the queue and the lock of choice
  if(!initialize()) 
    return 1;
  #ifdef SPINLOCK
  struct spin_lock_t sl = {0};
  #endif
  #ifdef FAIRSPINLOCK
  struct spin_lock_t sl = {0, 0};
  #endif

  // Get ready to create threads, inform user.
  pthread_t threads[threadCount];
  time(&startTime);
  printf("Starting up %i producers and %i consumers for %i seconds.\n", proThreadCount, conThreadCount, runTime);

  // Create the producer threads.
  for(i = 0; i < proThreadCount; i++)
    {
      // Pass an argument that depends on the lock.
      #ifdef BAKERY
      void *arg = (void*)(intptr_t)i;
      #endif
      #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
      void *arg = (void*)&sl;
      #endif
      #ifdef MUTEX
      void *arg = (void*)&lock;
      #endif
      // Attempt to create the thread.
      if(pthread_create(&threads[i], NULL, proThread, arg))
	{
	  printf("Error creating thread.\n");
	  return 1;
	}
    }
  // Create the consumer threads.
  for(i = proThreadCount; i < threadCount; i++)
    {
      // Pass a lock dependent argument.
      #ifdef BAKERY
      void *arg = (void*)(intptr_t)i;
      #endif
      #if defined(SPINLOCK) || defined(FAIRSPINLOCK)
      void *arg = (void*)&sl;
      #endif
      #ifdef MUTEX
      void *arg = (void*)&lock;
      #endif
      // Attempt to create the thread.
      if(pthread_create(&threads[i], NULL, conThread, arg))
	{
	  printf("Error creating thread.\n");
	  return 1;
	}
    }

  // Store producer results.
  int proResults[proThreadCount];
  // Join the producer threads.
  for(i = 0; i < proThreadCount; i++)
    {
      void *status;
      if(pthread_join(threads[i], &status))
	{
	  printf("Error joining thread.\n");
	  return 1;
	}
      printf("Producer %i:\t%li\n", i, (intptr_t)status);
      proResults[i] = (intptr_t)status;
    }
  // Calculate statistics on the producer threads.
  float stdDev = std_dev(proResults, proThreadCount);
  printf("StdDev:\t\t%i\n\n", (int)stdDev);

  // Store consumer results.
  int conResults[conThreadCount];
  // Join the consumer threads.
  for(i = proThreadCount; i < threadCount; i++)
    {
      void *status;
      if(pthread_join(threads[i], &status))
	{
	  printf("Error joining thread.\n");
	  return 1;
	}
      printf("Consumer %i:\t%li\n", i, (intptr_t)status);
      conResults[i-proThreadCount] = (intptr_t)status;
    }
  // Calculate statistics on the consumer threads.
  stdDev = std_dev(conResults, conThreadCount);
  printf("StdDev:\t\t%i\n\n", (int)stdDev);

  // Delete the mutex if used.
  #ifdef MUTEX
  pthread_mutex_destroy(&lock);
  #endif
  return 0;
}


