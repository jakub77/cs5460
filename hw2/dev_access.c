#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main (int argc, char *argv[])
{
  // Make sure we have the right number of arguments.
  if(argc != 2)
    {
      printf("Expects one argument: \"0\" or \"1\"\n");
      return 0;
    }

  // Parse the command argument into an int.
  int arg = atoi(argv[1]);

  // If it is 0, we read from mouse.
  if(arg == 0)
    {
      // Open the file and check for errors.
      unsigned char buffer;
      FILE *pFile;
      pFile = fopen("/dev/input/mouse2", "r");
      if(pFile == NULL)
	{
	  printf("/dev/input/mouse0 could not be opened. (running as root?)\n");
	  return 0;
	}

      // Loop infinately.
      for(;;)
	{
	  size_t res = fread(&buffer, 1, 1, pFile);
	  if(res != 1)
	    {
	      printf("Reading error.");
	      return 0;
	    }
	  printf("%i\n", (int)buffer);
	}
    }
  else if(arg == 1)
    {
      FILE *fin;
      fin = fopen("/dev/urandom", "r");
      if(fin == NULL)
	{
	  printf("/dev/urandom could not be opened.\n");
	  return 0;
	}
      FILE *fout;
      fout = fopen("/dev/null", "w");
      if(fout == NULL)
	{
	  printf("/dev/null could not be opened.\n");
	  return 0;
	}

      size_t totalBytes = 10000000;
      size_t bytesRead = 0;
      size_t bytesWritten = 0;
      size_t bytesToRead = totalBytes;
      size_t bytesToWrite = totalBytes;
      struct timeval tim;
      double t1;
      double t2;

      char *buffer = (char*)malloc(totalBytes);
      if(buffer == NULL)
	{
	  printf("malloc failed.\n");
	  return 0;
	}

      gettimeofday(&tim, NULL);
      t1 = tim.tv_sec + (tim.tv_usec/1000000.0);

      while(bytesToRead > 0)
	{
	  size_t res = fread((void*)(buffer+bytesRead), 1, bytesToRead, fin);
	  bytesRead += res;
	  bytesToRead -= res;
	}

      while(bytesToWrite > 0)
	{
	  size_t res = fwrite((void*)(buffer+bytesWritten), 1, bytesToWrite, fout);
	  bytesWritten += res;
	  bytesToWrite -= res;
	}

      gettimeofday(&tim, NULL);
      t2 = tim.tv_sec + (tim.tv_usec/1000000.0);
      printf("%f seconds elapsed\n", t2-t1);
    }
  else
    {
      printf("Invalid argument %i, expected \"0\" or \"1\"\n", arg);
    }
  return 0;
}
