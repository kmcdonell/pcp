#
# Copyright (c) 2000,2004 Silicon Graphics, Inc.  All Rights Reserved.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

TOPDIR = ../..
include $(TOPDIR)/src/include/builddefs

CFILES = pmlogcheck.c pass0.c pass1.c pass2.c pass3.c
HFILES = logcheck.h
CMDTARGET = pmlogcheck$(EXECSUFFIX)
HELPER = logdecompress
LLDLIBS	= $(PCPLIB) $(LIB_FOR_MATH)

default:	$(CMDTARGET) $(HELPER)

include $(BUILDRULES)

install:	$(CMDTARGET)
	$(INSTALL) -m 755 $(CMDTARGET) $(PCP_BIN_DIR)/$(CMDTARGET)
	$(INSTALL) -m 755 $(HELPER) $(PCP_SHARE_DIR)/bin/$(HELPER)

default_pcp:	default

install_pcp:	install

$(OBJECTS):	logcheck.h

$(OBJECTS):	$(TOPDIR)/src/include/pcp/libpcp.h
