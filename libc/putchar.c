#include <stdio.h>
#include <unistd.h>

int putchar(int c)
{
  // write character to stdout
  if(write(1, &c, 1)) return c;
  else return -1;
}
