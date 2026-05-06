# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-logburst) begin
(fd-logburst) opened 48 burst handles
(fd-logburst) closed sparse burst handles
(fd-logburst) reused burst holes
(fd-logburst) read high burst fd
(fd-logburst) read reused burst fd
(fd-logburst) closed burst handles
(fd-logburst) end
fd-logburst: exit(0)
EOF
pass;
