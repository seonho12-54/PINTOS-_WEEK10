# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-expand) begin
(fd-expand) opened 40 files
(fd-expand) read first fd
(fd-expand) read expanded fd
(fd-expand) closed 40 files
(fd-expand) end
fd-expand: exit(0)
EOF
pass;
