# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-browser) begin
(fd-browser) opened 24 tab handles
(fd-browser) reused closed tab fds
(fd-browser) read first active tab
(fd-browser) read reopened tab
(fd-browser) closed browser handles
(fd-browser) end
fd-browser: exit(0)
EOF
pass;
