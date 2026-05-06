/* Simulates a CPU-only command that never opens a file.
   Dynamic fd tables should not allocate table storage for this case. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  msg ("cpu-only command does not open files");
  msg ("fd table should stay unused");
}
