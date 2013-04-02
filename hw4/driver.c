#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
  // Make sure we have the right number of arguments.
  if(argc != 3)
    {
      printf("Expects two arguments\n");
      return 0;
    }

  // Parse the command argument into an int.
  char sleepyNumber = argv[1][0];
  int arg = atoi(argv[2]);
  char sleepyName[] = "/dev/sleepyX";
  sleepyName[11] = sleepyNumber;

  size_t fileDes = open(sleepyName, O_RDWR);
  if(fileDes <= 0)
    {
      printf("Could not open %s\n", sleepyName);
      return 0;
    }

  printf("Using '%s'\n", sleepyName);
  int res;
  if(arg != -1)
    res = write(fileDes, &arg, 4);
  else
    res = read(fileDes, &arg, 4);
  printf("Got a result of %i seconds\n", res);
  return 0;

}
