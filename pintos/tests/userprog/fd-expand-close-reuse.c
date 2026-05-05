/* Opens enough files to grow the fd table, closes holes, then checks
   that later opens reuse the closed descriptor numbers. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define OPEN_CNT 40
#define HOLE_CNT 3

void
test_main (void)
{
  int fds[OPEN_CNT];
  int holes[HOLE_CNT] = {5, 17, 32};
  int expected[HOLE_CNT];
  int reopened[HOLE_CNT];
  int i, j;

  for (i = 0; i < OPEN_CNT; i++)
    {
      fds[i] = open ("sample.txt");
      if (fds[i] < 2)
        fail ("open #%d returned %d", i, fds[i]);

      for (j = 0; j < i; j++)
        if (fds[j] == fds[i])
          fail ("open #%d reused fd %d too early", i, fds[i]);
    }

  msg ("opened %d files", OPEN_CNT);

  for (i = 0; i < HOLE_CNT; i++)
    {
      expected[i] = fds[holes[i]];
      close (expected[i]);
      fds[holes[i]] = -1;
    }

  msg ("closed holes");

  for (i = 0; i < HOLE_CNT; i++)
    {
      reopened[i] = open ("sample.txt");
      if (reopened[i] != expected[i])
        fail ("reopen #%d returned fd %d, expected %d",
              i, reopened[i], expected[i]);
    }

  msg ("reused closed fds");

  for (i = 0; i < HOLE_CNT; i++)
    close (reopened[i]);

  for (i = 0; i < OPEN_CNT; i++)
    if (fds[i] != -1)
      close (fds[i]);

  msg ("closed all files");
}
