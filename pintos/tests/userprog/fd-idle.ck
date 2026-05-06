# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-idle) begin
(fd-idle) cpu-only command does not open files
(fd-idle) fd table should stay unused
(fd-idle) end
fd-idle: exit(0)
EOF
pass;
