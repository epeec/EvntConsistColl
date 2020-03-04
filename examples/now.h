#ifndef NOW_H
#define NOW_H

#include <stdio.h>
#include <sys/time.h>

double now()
{
  struct timeval tp;

  if (gettimeofday (&tp, NULL) < 0)
  {
    perror ("gettimeofday failed");
  }

  return (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6;
}


#endif
