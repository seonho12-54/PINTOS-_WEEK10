/* Opens more files than the initial dynamic fd table capacity.
   This forces fd_table growth and verifies old and expanded descriptors work. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define OPEN_CNT 40

void
test_main (void)
{
  int fds[OPEN_CNT];
  char byte;
  int i, j;

  for (i = 0; i < OPEN_CNT; i++)
    {
      fds[i] = open ("sample.txt");
      if (fds[i] < 2)
        fail ("open #%d returned %d", i, fds[i]);

      for (j = 0; j < i; j++)
        if (fds[j] == fds[i])
          fail ("open #%d reused fd %d", i, fds[i]);
    }

  msg ("opened %d files", OPEN_CNT);

  CHECK (read (fds[0], &byte, 1) == 1, "read first fd");
  CHECK (read (fds[OPEN_CNT - 1], &byte, 1) == 1, "read expanded fd");

  for (i = 0; i < OPEN_CNT; i++)
    close (fds[i]);

  msg ("closed %d files", OPEN_CNT);
}
