// Jakub Szpunar cs5460 HW3 Problem 6: Create a queue.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

// pointer to the queue
int* queue;
// Max size
static const int MAX_QUEUE_SIZE = 32;
// Queue positions and size.
int enqPos;
int deqPos;
int curQueueSize;

// Enqueue value into the queue.
// Return 0 if fail due to full queue, 1 otherwise.
int enq(int value)
{
  // If the queue is full, return so.
  if(curQueueSize >= 32)
    return 0;
  
  // Insert the value in the next available position for inserting.
  queue[enqPos] = value;

  // Incremement the enqueue position with 
  // care for going off the end of the queue.
  if(enqPos == MAX_QUEUE_SIZE - 1)
    enqPos = 0;
  else
    enqPos++;

  // Incremement the queue size.
  curQueueSize++;
  return 1;
}

// Dequeue from the queue.
// Return 0 if failure do to tempty queue.
// If success, set *value to be the deqeued value and return 1.
int deq(int* value)
{
  // If the queue is empty.
  if(curQueueSize == 0)
    return 0;

  // Set value.
  (*value) = queue[deqPos];

  // Update next dequeue position.
  if(deqPos == MAX_QUEUE_SIZE - 1)
    deqPos = 0;
  else
    deqPos++;

  // decremement the queue size.
  curQueueSize--;
  return 1;
}

// Initalize the queue to be considered empty.
void initialize()
{
  enqPos = 0;
  deqPos = 0;
  curQueueSize = 0;
  return;
}

// Test case, fill up the queue and then 
// remove from the queue checking values.
int enqFullDeqFull()
{
  int i;
  for(i = 0; i < MAX_QUEUE_SIZE; i++)
    {
      if(!enq(i))
	return 0;
    }
  for(i = 0; i < MAX_QUEUE_SIZE; i++)
    {
      int val = -1;
      if(!deq(&val))
	return 0;
      if(val != i)
	return 0;
    }
  return 1;
}

// Test case, enqueue, then dequene, repeat many times
// checking values.
int enqDeqByOne()
{
  int i;
  for(i = 0; i < MAX_QUEUE_SIZE*10; i++)
    {
      if(!enq(i))
	return 0;
      int val = -1;
      if(!deq(&val))
	return 0;
      if(i != val)
	return 0;
    }
  return 1;
}

// Test case, make sure that the queue cannot be overfilled.
int enqTooMany()
{
  int i;
  for(i = 0; i < MAX_QUEUE_SIZE; i++)
    {
      if(!enq(i))
	return 0;
    }

  if(enq(i))
    return 0;
  return 1;
}

// Test case, make sure that the queue cannot be overemptied.
int deqTooMany()
{
  int i;
  if(deq(&i))
    return 0;
  return 1;
}

// Complex test case. Compute random data, then write to the queue
// in variable chunks. Read from the queue in variable chunks.
// Do so many times.
int enqDeqVariable()
{
  int iterations = 15000;
  int *expected = malloc(iterations*sizeof(int));
  if(expected == NULL)
    return -1;
  srand(time(NULL));
  int i;
  for(i = 0; i < iterations; i++)
    {
      expected[i] = rand();
    }

  int iterationsComplete = 0;
  int chunkSize = 0;
  for(; iterationsComplete < iterations;)
    {
      for(i = 0; i < chunkSize; i++)
	{
	  if(!enq(expected[iterationsComplete + i]))
	    return 0;
	}
      for( i = 0; i < chunkSize; i++)
	{
	  int val = -1;
	  if(!deq(&val))
	    return 0;
	  if(val != expected[iterationsComplete + i])
	    return 0;
	}
      iterationsComplete += chunkSize;
      if(chunkSize == MAX_QUEUE_SIZE - 1)
	chunkSize = 1;
      else
	chunkSize++;
    }
  return 1;
}

// Test case, fill the queue to full, then try to overfill.
// Next empty out half, refill to full again, then try to overfill.
int fillToFullDeqFill()
{
  int i = 0;
  for(i = 0; i < MAX_QUEUE_SIZE; i++)
      if(!enq(i))
	return 0;
 
  for(i = 0; i < MAX_QUEUE_SIZE; i++)
    if(enq(i))
      return 0;

  for(i = 0; i < MAX_QUEUE_SIZE/2; i++)
    {
      int val = -1;
      if(!deq(&val))
	return 0;
      if(val != i)
	return 0;
    }

  for(i = 0; i < MAX_QUEUE_SIZE/2; i++)
    {
      if(!enq(i))
	return 0;
    }

  if(enq(i))
    return 0;
  return 1;
}

// Main, creates the queue, then tests its correctness.
int main()
{ 
  // Malloc up the queue size.
  queue = malloc(MAX_QUEUE_SIZE*sizeof(int));
  if(queue == NULL)
    {
      printf("Malloc error.\n");
      return 1;
    }
  // Initialize the queue and run test cases on it.
  // Report the outcome of the test case to the user.
  initialize();
  if(!enqFullDeqFull())
    printf("Enque full deque full \tfailed\n");
  else
    printf("Enque full deque full \tsucceeded\n");

  initialize();
  if(!enqDeqByOne())
    printf("Enque deque by one \tfailed\n");
  else
    printf("Enque deque by on \tsucceeded\n");

  initialize();
  if(!enqTooMany())
    printf("Enque too many \t\tfailed\n");
  else
    printf("Enque too many \t\tsucceeded\n");

  initialize();
  if(!deqTooMany())
    printf("Deque too many \t\tfailed\n");
  else
    printf("Deque too many \t\tsucceeded\n");

  initialize();
  if(!enqDeqVariable())
    printf("Enque deque variable \tfailed\n");
  else
    printf("Enque deque variable \tsucceeded\n");

  initialize();
  if(!fillToFullDeqFill())
    printf("Fill to full deq fill \tfailed\n");
  else
    printf("Fill to full deq fill \tsucceeded\n");
  return 0;
}


