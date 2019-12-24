#
#   Carbon Framework
#   Third party libraries
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 19.01.2017 17:29:24
#	Initial revision.
#

include ../tool/config.mak

_MODS1 = $(MODULE_DEP)
-include $(patsubst %, ../src/module/%/depend.mak, $(_MODS1))
_MODS2 = $(MODULE_DEP)
-include $(patsubst %, ../src/module/%/depend.mak, $(_MODS2))

THPARTY_DEP := $(sort $(THPARTY_DEP))

#_DIRS = $(shell find ../module -maxdepth 1 -type d)

# Variable THPARTY_DEP contains list of unique third party dependent libraries

#xxx:
#	echo XXX=$(MAKECMDGOALS)
#	echo _MOD_DIRS=$(_DIRS)
#	echo DEP_MODS=$(MODULE_DEP)
#	echo TH_DEP=$(THPARTY_DEP)

# jemalloc-4.1.0 pugixml-1.6 sqlite-3081101 udns-0.4 jansson-2.7 mp4v2-2.0.0

_DIRS_ALL := jemalloc pugixml sqlite udns jansson mp4v2 zlib

ifeq ($(MAKECMDGOALS), cleanall)
DIRS := $(_DIRS_ALL)
else ifeq ($(MAKECMDGOALS), makeall)
DIRS := $(_DIRS_ALL)
else
DIRS := $(THPARTY_DEP)
endif

include ../tool/thparty.mak
