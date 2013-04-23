#include <stdio.h>

int main(int argc, char* argv[])
{
  printf("Setting: 123, 45\n");
  int x = set_ext(123, 45);
  printf("Result : %i, %i\n", get_ext_pos(x), get_ext_count(x));

  printf("Setting: 9876543, 21\n");
  x = set_ext(9876543, 21);
  printf("Result : %i, %i\n", get_ext_pos(x), get_ext_count(x));

  printf("Setting: 77, 513\n");
  x = set_ext(77, 513);
  printf("Result : %i, %i\n", get_ext_pos(x), get_ext_count(x));
  return 0;
}

int get_ext_pos(int x)
{
  return ((x >> 8) & 0xFFFFFF);
}

int get_ext_count(int x)
{
  return x & 0xFF;
}
int set_ext(int pos, int count)
{
  count = count & 0xFF;
  pos = (pos & 0xFFFFFF) << 8;
  return pos | count;
}
