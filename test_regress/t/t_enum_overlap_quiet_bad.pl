#!/usr/bin/perl
if (!$::Driver) { use FindBin; exec("$FindBin::Bin/bootstrap.pl", @ARGV, $0); die; }
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2019 by Todd Strader. This program is free software; you can
# redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.

scenarios(vlt => 1);

top_filename("t/t_enum_overlap_bad.v");

# Tests for the error message and then the absence of the
# "Command Failed" line
compile(
    v_flags2 => ["--quiet-exit --lint-only"],
    fails => 1,
    expect =>
'%Error: t/t_enum_overlap_bad.v:\d+: Overlapping enumeration value: e1b
%Error: t/t_enum_overlap_bad.v:\d+: ... Location of original declaration
%Error: Exiting due to 1 error\(s\)
((?!Command Failed).)*$',
    );

ok(1);
1;
