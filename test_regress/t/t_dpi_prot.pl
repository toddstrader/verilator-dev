#!/usr/bin/perl
if (!$::Driver) { use FindBin; exec("$FindBin::Bin/bootstrap.pl", @ARGV, $0); die; }
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2019 by Todd Strader. This program is free software; you can
# redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.

scenarios(simulator => 1);

# Always compile the secret file with Verilator no matter what simulator
#   we are testing with
compile (
    atsim => 0,
    ghdl => 0,
    vcs => 0,
    nc => 0,
    ms => 0,
    iv => 0,
    xsim => 0,
    vlt => 1,
    vlt_all => 1,
    verilator_flags2 => ["--dpi-protect dpi_prot_secret"],
    top_filename => "t_dpi_prot_secret.v",
    VM_PREFIX => "Vt_dpi_prot_secret",
    verilator_make_gcc => 0,
    );

$Self->_run(logfile=>"$Self->{obj_dir}/dpi_prot_gcc.log",
            cmd=>["make",
                  "-C", "$Self->{obj_dir}",
                  "-f", "Vt_dpi_prot_secret.mk"]);

# TODO -LDLIBS doesn't work
compile(
    verilator_flags2 => ["-LDFLAGS '-L. -ldpi_prot_secret'"],
    );

execute(
    check_finished => 1,
    );

ok(1);
1;
