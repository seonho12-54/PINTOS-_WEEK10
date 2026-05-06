# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-exp-reuse) begin
(fd-exp-reuse) opened 40 files
(fd-exp-reuse) closed holes
(fd-exp-reuse) reused closed fds
(fd-exp-reuse) closed all files
(fd-exp-reuse) end
fd-exp-reuse: exit(0)
EOF
pass;
