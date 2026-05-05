/* Opens enough files to grow the fd table, then forks and verifies that
   the expanded table is duplicated into the child process. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define OPEN_CNT 40

void
test_main (void)
{
  int fds[OPEN_CNT];
  char byte;
  int i;
  int pid;

  for (i = 0; i < OPEN_CNT; i++)
    {
      fds[i] = open ("sample.txt");
      if (fds[i] < 2)
        fail ("open #%d returned %d", i, fds[i]);
    }

  msg ("parent opened %d files", OPEN_CNT);

  pid = fork ("fd-expand-child");
  if (pid < 0)
    fail ("fork returned %d", pid);

  if (pid == 0)
    {
      CHECK (read (fds[OPEN_CNT - 1], &byte, 1) == 1,
             "child read expanded fd");

      int extra = open ("sample.txt");
      if (extra < 2)
        fail ("child extra open returned %d", extra);

      msg ("child opened extra fd");
      close (extra);
      exit (57);
    }
  else
    {
      CHECK (wait (pid) == 57, "child exit status is 57");
      CHECK (read (fds[OPEN_CNT - 1], &byte, 1) == 1,
             "parent read expanded fd");

      for (i = 0; i < OPEN_CNT; i++)
        close (fds[i]);
    }
}
