// Jakub Szpunar
// CS5460
// HW2
// Feb 2013

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
  // Make sure we have the right number of arguments.
  if(argc != 2)
    {
      printf("Expects one argument: \"0\" or \"1\" or \"2\"\n");
      return 0;
    }

  // Parse the command argument into an int.
  char* end;
  int arg = strtol(argv[1], &end, 10);
  // Check to see if a 0 arg means strtol failed, or actual 0 returned.
  if(end == argv[1] || *end != '\0' || errno == ERANGE)
    {
      printf("Expects one argument: \"0\" or \"1\" or \"2\"\n");
      return 0;
    }

  // If it is 0, we read from mouse.
  if(arg == 0)
    {
      // Buffer for data.
      char buffer;

      // Open the file and check for errors in opening.
      size_t fileDes = open("/dev/input/mouse2", O_RDONLY);
      if(fileDes <= 0)
	{
	  printf("/dev/input/mouse0 could not be opened. (running as root?)\n");
	  return 0;
	}

      // Loop infinately reading a single bytes and printing it out.
      // If a byte cannot be read, inform user of error.
      for(;;)
	{
	  size_t res = read(fileDes, &buffer, 1);
	  if(res != 1)
	    {
	      printf("Reading error. (running as root?)\n");
	      return 0;
	    }
	  printf("%i\n", (int)buffer);
	}
    }
  // If it is 1, we read from urandom and write to null.
  else if(arg == 1)
    {
      // Attempt to open files for reading and writing.
      size_t fin = open("/dev/urandom", O_RDONLY);
      if(fin <= 0)
	{
	  printf("/dev/urandom could not be opened for reading.\n");
	  return 0;
	}

      size_t fout = open("/dev/null", O_WRONLY);
      if(fin <= 0)
	{
	  printf("/dev/null could not be opened for writing.\n");
	  return 0;
	}

      // Define variables for the byte counts, and timing.
      size_t totalBytes = 10000000;
      size_t bytesRead = 0;
      size_t bytesWritten = 0;
      size_t bytesToRead = totalBytes;
      size_t bytesToWrite = totalBytes;
      struct timeval tim;
      double t1;
      double t2;

      // Malloc a buffer, and check for errors.
      char *buffer = (char*)malloc(totalBytes);
      if(buffer == NULL)
	{
	  printf("malloc failed.\n");
	  return 0;
	}

      // Start timing.
      gettimeofday(&tim, NULL);
      t1 = tim.tv_sec + (tim.tv_usec/1000000.0);

      // Read totalBytes bytes.
      while(bytesToRead > 0)
	{
	  size_t res = read(fin, (buffer+bytesRead), bytesToRead);
	  // Check for errors.
	  if((int)res < 0)
	    {
	      printf("A read error occured.\n");
	      return 0;
	    }
	  // Update how many more bytes need to be read.
	  bytesRead += res;
	  bytesToRead -= res;
	}

      // Write totalBytes bytes
      while(bytesToWrite > 0)
	{
	  size_t res = write(fout, (buffer+bytesWritten), bytesToWrite);
	  // Check for errors.
	  if((int)res < 0)
	    {
	      printf("A read error occured.\n");
	      return 0;
	    }
	  // Update how many more bytes need to be written.
	  bytesWritten += res;
	  bytesToWrite -= res;
	}

      // Finish timing, and output the total number of seconds elapsed.
      gettimeofday(&tim, NULL);
      t2 = tim.tv_sec + (tim.tv_usec/1000000.0);
      printf("%f seconds elapsed\n", t2-t1);
    }
  else if(arg == 2)
    {
       // Buffer for data.
      int buffer, i;

      // Open the file and check for errors in opening.
      size_t fileDes = open("/dev/ticket0", O_RDONLY);
      if(fileDes <= 0)
	{
	  printf("/dev/ticket0 could not be opened.\n");
	  return 0;
	}
      
      // Loop 10 times getting a ticket number.
      for(i = 0; i < 10; i++)
	{
	  // Get the ticket number.
	  int res = read(fileDes, &buffer, 4);
	  // If we don't get 4 bytes, and error occured.
	  if(res != 4)
	    {
	      printf("read() reaturned %d, errno is %d\n", res, errno);
	      return 0;
	    }
	  // Print the ticket number.
	  printf("%i\n", buffer);
	  // Sleep for a second.
	  sleep(1);
	}
    }
  // Invalid arg given.
  else
    {
      printf("Invalid argument %i, expected \"0\" or \"1\" or \"2\"\n", arg);
    }
  return 0;
}
