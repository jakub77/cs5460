// Jakub Szpunar cs5460 HW3 Problem 1: Single thread bakery algorithm.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

// pointer to the queue.
int* queue;
static const int MAX_QUEUE_SIZE = 32;
int enqPos;
int deqPos;
int curQueueSize;

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

int deqTooMany()
{
  int i;
  if(deq(&i))
    return 0;
  return 1;
}

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

int main()
{ 
  queue = malloc(MAX_QUEUE_SIZE*sizeof(int));
  if(queue == NULL)
    {
      printf("Malloc error.\n");
      return 1;
    }
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


