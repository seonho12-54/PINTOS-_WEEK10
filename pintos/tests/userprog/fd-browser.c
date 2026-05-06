/* Simulates a browser-like workload: many open tabs, a few tabs closed,
   and new tabs reusing the lowest available file descriptors. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define TAB_CNT 24
#define HOLE_CNT 3

void
test_main (void)
{
  static const int holes[HOLE_CNT] = {4, 9, 18};
  int fds[TAB_CNT];
  int expected[HOLE_CNT];
  int reopened[HOLE_CNT];
  char byte;
  int i, j;

  for (i = 0; i < TAB_CNT; i++)
    {
      fds[i] = open ("sample.txt");
      if (fds[i] < 2)
        fail ("tab open #%d returned %d", i, fds[i]);
    }

  msg ("opened %d tab handles", TAB_CNT);

  for (i = 0; i < HOLE_CNT; i++)
    {
      expected[i] = fds[holes[i]];
      close (fds[holes[i]]);
      fds[holes[i]] = -1;
    }

  for (i = 0; i < HOLE_CNT; i++)
    {
      reopened[i] = open ("sample.txt");
      if (reopened[i] != expected[i])
        fail ("new tab got fd %d instead of reusable fd %d",
              reopened[i], expected[i]);
    }

  msg ("reused closed tab fds");

  CHECK (read (fds[0], &byte, 1) == 1, "read first active tab");
  CHECK (read (reopened[HOLE_CNT - 1], &byte, 1) == 1,
         "read reopened tab");

  for (i = 0; i < TAB_CNT; i++)
    if (fds[i] != -1)
      close (fds[i]);

  for (j = 0; j < HOLE_CNT; j++)
    close (reopened[j]);

  msg ("closed browser handles");
}
