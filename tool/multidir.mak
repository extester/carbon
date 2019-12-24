#
#   Carbon Framework
#   Make script for multi-directory compilation
#
#   Copyright (c) 2016 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 23.06.2016 10:29:19
#	Initial revision.
#
#   Input variables:
#	DIRS=<dir_list>
#
#	CROSS_COMPILE="gcc_prefix"
#	MACHINE="machine"
#	RELEASE=1 
#

MAKE ?= make

CMD_DEF=
ifdef CROSS_COMPILE
CMD_DEF+=CROSS_COMPILE=$(CROSS_COMPILE)
endif

ifdef MACHINE
CMD_DEF+=MACHINE=$(MACHINE)
endif
 
ifdef RELEASE
CMD_DEF+=RELEASE=1
endif


.PHONY: all clean r rebuild makeall cleanall

all makeall:
	for path in $(DIRS); do \
	    echo "making $@ in `pwd`/$$path"; \
	    (cd $$path; $(MAKE) $(CMD_DEF) $@) || exit 1; \
	done

clean cleanall:
	for path in $(DIRS); do \
	    echo "making $$@ in `pwd`/$$path"; \
	    (cd $$path; $(MAKE) $(CMD_DEF) $@) || exit 1; \
	done

r rebuild: clean all

