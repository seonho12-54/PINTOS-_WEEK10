# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-expand-close-reuse) begin
(fd-expand-close-reuse) opened 40 files
(fd-expand-close-reuse) closed holes
(fd-expand-close-reuse) reused closed fds
(fd-expand-close-reuse) closed all files
(fd-expand-close-reuse) end
fd-expand-close-reuse: exit(0)
EOF
pass;
