#!/usr/bin/perl
if (!$::Driver) { use FindBin; exec("$FindBin::Bin/bootstrap.pl", @ARGV, $0); die; }
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2019 by Todd Strader. This program is free software; you can
# redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.

scenarios(
    vlt => 1,
    xsim => 1,
    );

my $cmd = "t/t_dpi_prot_secret.pl --vlt";
my $secret_prefix = "t_dpi_prot_secret";

# Always compile the secret file with Verilator no matter what simulator
#   we are testing with
if ($Self->{xsim}) {
    $cmd = "t/t_dpi_prot_secret_shared.pl --vlt";
    $secret_prefix = "t_dpi_prot_secret_shared";
}

my $secret_dir = "$Self->{obj_dir}/../../obj_vlt/$secret_prefix";

die "Could not build secret library" if system($cmd);

compile(
    verilator_flags2 => ["$secret_dir/secret.sv",
                         " -LDFLAGS",
                         "'-L../$secret_prefix -lsecret'"],
    xsim_flags2 => ["$secret_dir/secret.sv"],
    );

execute(
    check_finished => 1,
    xsim_run_flags2 => ["--sv_lib",
                        "$secret_dir/libsecret",
                        "--dpi_absolute"],
    );

ok(1);
1;
