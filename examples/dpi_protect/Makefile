######################################################################
#
# DESCRIPTION: Verilator Example: --dpi-protect Makefile
#
# This calls the object directory makefiles.  That allows the objects to
# be placed in the "current directory" which simplifies the Makefile.
#
# Copyright 2019 by Todd Strader. This program is free software; you can
# redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
#
######################################################################
# Check for sanity to avoid later confusion

ifneq ($(words $(CURDIR)),1)
 $(error Unsupported: GNU Make cannot build in directories containing spaces, build elsewhere: '$(CURDIR)')
endif

######################################################################
# Set up variables

# If $VERILATOR_ROOT isn't in the environment, we assume it is part of a
# package install, and verilator is in your path. Otherwise find the
# binary relative to $VERILATOR_ROOT (such as when inside the git sources).
ifeq ($(VERILATOR_ROOT),)
VERILATOR = verilator
else
export VERILATOR_ROOT
VERILATOR = $(VERILATOR_ROOT)/bin/verilator
endif

VERILATOR_FLAGS =
# Generate C++
VERILATOR_FLAGS += -cc
# Optimize
VERILATOR_FLAGS += -O2 -x-assign 0
# Warn abount lint issues; may not want this on less solid designs
VERILATOR_FLAGS += -Wall
# Make waveforms
VERILATOR_FLAGS += --trace

######################################################################
default: run

run:
	@echo
	@echo "-- Verilator --dpri-protect example ------"

# --dpi-protect-shared can be used to produce a shared library
# instead of this static library.  Some simulators may require
# the shared library.
	@echo
	@echo "-- VERILATE secret module ----------------"
	$(VERILATOR) $(VERILATOR_FLAGS) --dpi-protect secret -Mdir obj_dir_secret/ secret_impl.v

# This builds secret.sv and libsecret.a which can be distributed
# apart from the source
	@echo
	@echo "-- COMPILE protected library -------------"
	$(MAKE) -j 4 -C obj_dir_secret -f Vsecret_impl.mk

# Use the SystemVerilog wrapper (secret.sv) and the library
# (libsecret.a) generated from the previous step
	@echo
	@echo "-- VERILATE top module -------------------"
	$(VERILATOR) $(VERILATOR_FLAGS) --exe -LDFLAGS '-L../obj_dir_secret -lsecret' top.v obj_dir_secret/secret.sv sim_main.cpp

	@echo
	@echo "-- COMPILE entire design -----------------"
	$(MAKE) -j 4 -C obj_dir -f Vtop.mk

	@echo
	@echo "-- RUN -----------------------------------"
	@mkdir -p logs
	obj_dir/Vtop +trace

	@echo
	@echo "-- DONE ----------------------------------"
	@echo "To see waveforms, open logs/vlt_dump.vcd in a waveform viewer"
	@echo


######################################################################
# Other targets

show-config:
	$(VERILATOR) -V

maintainer-copy::
clean mostlyclean distclean maintainer-clean::
	-rm -rf obj_dir* logs *.log core
