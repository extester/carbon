#
#   Carbon Framework
#   Make script for third-party software compilation
#
#   Copyright (c) 2016 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 30.03.2016 17:43:21
#	Initial revision.
#
#   Optional input variables:
#	CROSS_COMPILE=<gcc_prefix> for cross compilation    
#

MAKE ?= make

XHOST=$(CROSS_COMPILE)
# Remove trailing '-'
HOST=$(XHOST:-=)

XPREFIX=$(CURDIR)/__root__

CONF_DEF=CROSS_COMPILE=$(CROSS_COMPILE) CC=$(CROSS_COMPILE)gcc
CONF_OPT=--host=$(HOST) --prefix=$(XPREFIX)
MAKE_OPT=CROSS_COMPILE=$(CROSS_COMPILE) PREFIX=$(XPREFIX)
INST_DEF=PREFIX=$(XPREFIX)

.PHONY: all clean r rebuild makeall cleanall

all makeall:
	for path in $(DIRS); do \
		cd $$path; \
		if [ -x ./RUN_CONFIGURE ]; then \
			$(CONF_DEF) ./RUN_CONFIGURE $(CONF_OPT) || exit 1; \
		else \
	 		[ -x ./configure ] && ($(CONF_DEFS) ./configure $(CONF_OPT) || exit 1); \
		fi; \
		if [ -x ./RUN_MAKE ]; then \
	 		./RUN_MAKE $(MAKE_OPT) || exit 1; \
		else \
	 		$(MAKE) $(MAKE_OPT) || exit 1; \
		fi; \
		if [ -x ./RUN_INSTALL ]; then \
			$(INST_DEF) ./RUN_INSTALL || exit 1; \
		elif [ -x ./RUN_MAKE ]; then \
			./RUN_MAKE $(MAKE_OPT) install || exit 1; \
		else \
	 		$(MAKE) $(MAKE_OPT) install || exit 1; \
		fi; \
		cd ..; \
	done

clean cleanall:
	[ -z "$(XPREFIX)" ] || rm -rf $(XPREFIX)/*
	-for path in $(DIRS); do \
		cd $$path; \
		if [ -x ./RUN_MAKE ]; then \
	 		./RUN_MAKE clean distclean; \
		else \
	 		$(MAKE) clean distclean; \
		fi; \
		cd ..; \
	done

r rebuild: clean all
