# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-expand-fork) begin
(fd-expand-fork) parent opened 40 files
(fd-expand-fork) child read expanded fd
(fd-expand-fork) child opened extra fd
fd-expand-child: exit(57)
(fd-expand-fork) child exit status is 57
(fd-expand-fork) parent read expanded fd
(fd-expand-fork) end
fd-expand-fork: exit(0)
EOF
pass;
