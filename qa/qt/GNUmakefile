#
# Copyright (c) 2014,2024 Red Hat.
#

TOPDIR = ../..
include $(TOPDIR)/src/include/builddefs

TESTDIR = $(PCP_VAR_DIR)/testsuite/qt
SUBDIRS = qmc_context qmc_desc qmc_dynamic qmc_event qmc_format \
	  qmc_group qmc_hosts qmc_indom qmc_metric qmc_source \
	  qtprobe

default setup default_pcp: $(SUBDIRS)
	$(SUBDIRS_MAKERULE)

install install_pcp: $(SUBDIRS)
	$(INSTALL) -m 755 -d $(TESTDIR)
	$(INSTALL) -m 644 -f GNUmakefile.install $(TESTDIR)/GNUmakefile
	$(INSTALL) -m 755 -f setup-executables $(TESTDIR)/setup-executables
	$(SUBDIRS_MAKERULE)

include $(BUILDRULES)
