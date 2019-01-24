#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>


int kbhit()
{
  int i;
  ioctl(0, FIONREAD, &i);
  return i;
}

int readlines()
{
  int i = 0;
  int c = ' ';
  system("stty raw -echo");
  printf("enter '*' to quit \n");
  
  for(;c!='*';i++)
  { 
    if (kbhit())
    {
      c=getchar();
      printf("\n got %c, on iteration %d", c, i);
    }
  }

  system("stty cooked echo");
  
  return 0;
}   

	
