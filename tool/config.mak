#
#   Carbon Framework
#   Master configuration file
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 09.02.2017 10:44:53
#	Initial revision.
#
#
#   Input variables:
#	MACHINE="machine-backend"	default is "unix"
#	CROSS_COMPILE="gcc_prefix"	
#	RELEASE=1
#
#
################################################################################
#
#   Configuration options:
#
#	-	unavailable
#	+	supported
#	e	supported, enabled by default
#	d	supported, disabled by default
#	N	supported, default value is N
#
#
#   Option				Unix		Embed
#   ---------------------------------------------------------------------
#
#   CARBON_JEMALLOC			e		-
#   CARBON_UDNS				e		-
#
#   CARBON_OBJECT_NAME_LENGTH		48		16
#   CARBON_LOGGER_BUFFER_LENGTH		512		128
#
#   CARBON_DEBUG_DUMP			+		+
#   CARBON_DEBUG_EVENT			+		+
#   CARBON_DEBUG_TRACK_OBJECT		+		+
#   CARBON_DEBUG_ASSERT			+		+
#
#   CARBON_TIMER_COUNT			-		2
#   CARBON_EVENT_COUNT			-		4
#
#   CARBON_THREAD_COUNT			-		2
#   CARBON_THREAD_STACK_SIZE		-		256
#   CARBON_THREAD_PRIORITY		-		6
#
################################################################################


# Target machine: unix, embed-<backend>
# Default is unix
MACHINE ?= unix

_x=$(subst -, ,$(MACHINE))
CARBON_MACHINE=$(word 1, $(_x))
CARBON_BACKEND=$(word 2, $(_x))
CARBON_BACKEND_VARIANT=$(word 3, $(_x))

CARBON_PATH :=$(abspath $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/..)
TOOL_PATH :=$(CARBON_PATH)/tool

ifeq ($(wildcard $(CARBON_PATH)/config.mak),)
$(error No configuration file config.mak found)
endif    

include $(CARBON_PATH)/config.mak
include $(TOOL_PATH)/config_$(CARBON_MACHINE).mak

__build_ver=debug
ifeq ($(RELEASE),1)
__build_ver=release
endif
BUILD_PATH:=$(CARBON_PATH)/build/$(MACHINE)_$(__build_ver)

#
# Options
#

ifndef CARBON_DEBUG_DUMP
ifeq ($(RELEASE),1)
CARBON_DEBUG_DUMP=0
else
CARBON_DEBUG_DUMP=1
endif
endif

ifndef CARBON_DEBUG_EVENT
ifeq ($(RELEASE),1)
CARBON_DEBUG_EVENT=0
else
CARBON_DEBUG_EVENT=1
endif
endif

ifndef CARBON_DEBUG_TRACK_OBJECT
ifeq ($(RELEASE),1)
CARBON_DEBUG_TRACK_OBJECT=0
else
CARBON_DEBUG_TRACK_OBJECT=1
endif
endif

ifndef CARBON_DEBUG_ASSERT
ifeq ($(RELEASE),1)
CARBON_DEBUG_ASSERT=0
else
CARBON_DEBUG_ASSERT=1
endif
endif

