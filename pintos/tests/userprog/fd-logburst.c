/* Simulates a service that briefly opens many log/input handles.
   This expands the dynamic fd table, then closes most handles and reuses holes. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define LOG_CNT 48
#define REOPEN_CNT 8

void
test_main (void)
{
  int fds[LOG_CNT];
  int expected[REOPEN_CNT];
  int reopened[REOPEN_CNT];
  char byte;
  int i;

  for (i = 0; i < LOG_CNT; i++)
    {
      fds[i] = open ("sample.txt");
      if (fds[i] < 2)
        fail ("log open #%d returned %d", i, fds[i]);
    }

  msg ("opened %d burst handles", LOG_CNT);

  for (i = 0; i < REOPEN_CNT; i++)
    {
      int idx = i * 3;
      expected[i] = fds[idx];
      close (fds[idx]);
      fds[idx] = -1;
    }

  msg ("closed sparse burst handles");

  for (i = 0; i < REOPEN_CNT; i++)
    {
      reopened[i] = open ("sample.txt");
      if (reopened[i] != expected[i])
        fail ("reopen #%d got fd %d instead of %d",
              i, reopened[i], expected[i]);
    }

  msg ("reused burst holes");

  CHECK (read (fds[LOG_CNT - 1], &byte, 1) == 1, "read high burst fd");
  CHECK (read (reopened[0], &byte, 1) == 1, "read reused burst fd");

  for (i = 0; i < LOG_CNT; i++)
    if (fds[i] != -1)
      close (fds[i]);

  for (i = 0; i < REOPEN_CNT; i++)
    close (reopened[i]);

  msg ("closed burst handles");
}
