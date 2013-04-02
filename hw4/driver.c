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
  int arg = atoi(argv[1]);
  size_t fileDes = open("/dev/sleepy0", O_RDWR);
  if(fileDes <= 0)
    {
      printf("Could not open /dev/sleepy0\n");
      return 0;
    }
  int res = write(fileDes, &arg, 4);
  printf("Got a result of %i seconds\n", res);
  return 0;

}
