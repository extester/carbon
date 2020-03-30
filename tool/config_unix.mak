#
#   Carbon Framework
#   UNIX machine configuration file
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 09.02.2017 11:32:25
#	Initial revision.
#
#
#   Input variables:
#	CARBON_MACHINE="machine"
#	CARBON_BACKEND=""
#

ifndef CARBON_MALLOC
CARBON_MALLOC=0
endif

ifndef CARBON_UDNS
CARBON_UDNS=1
endif
ifeq ($(CARBON_UDNS),1)
THPARTY_DEP += udns
endif

ifndef CARBON_JANSSON
CARBON_JANSSON=0
endif
ifeq ($(CARBON_JANSSON),1)
THPARTY_DEP += jansson
endif

ifndef CARBON_XML
CARBON_XML=0
endif
ifeq ($(CARBON_XML),1)
THPARTY_DEP += pugixml
endif

ifeq ($(CARBON_ZLIB),1)
THPARTY_DEP += zlib
endif

ifndef CARBON_OBJECT_NAME_LENGTH
CARBON_OBJECT_NAME_LENGTH=48
endif

ifndef CARBON_LOGGER_BUFFER_LENGTH
CARBON_LOGGER_BUFFER_LENGTH=384
endif

ifdef CARBON_TIMER_COUNT
$(error, "can't use CARBON_TIMER_COUNT for UNIX machine")
endif

ifdef CARBON_EVENT_COUNT
$(error, "can't use CARBON_EVENT_COUNT for UNIX machine")
endif

ifdef CARBON_THREAD_COUNT
$(error, "CARBON_THREAD_COUNT is not supported for UNIX machine")
endif

ifdef CARBON_THREAD_STACK_SIZE
$(error, "CARBON_THREAD_STACK_SIZE is not supported for UNIX machine")
endif

ifdef CARBON_THREAD_PRIORITY
$(error, "CARBON_THREAD_PRIORITY is not supported for UNIX machine")
endif

