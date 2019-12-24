#
#   Carbon framework TNeo backend makefile
#
#   Copyright (c) 2018 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 03.10.2018
#	Initial revision.
#

MAKE ?= make

all makeall:
	(cd tneo/tneo; $(MAKE) TN_ARCH="cortex_m3" TN_COMPILER="$(CROSS_COMPILE)gcc")

clean cleanall:
	(cd tneo/tneo; rm -Rf _obj bin)

r rebuild: clean all
