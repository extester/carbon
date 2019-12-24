#
#   Carbon Framework
#   EMBED machine configuration file
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 15.02.2017 12:00:24
#	Initial revision.
#
#
#   Input variables:
#	CARBON_MACHINE="machine"
#	CARBON_BACKEND="backend"
#

CARBON_JEMALLOC=0
CARBON_UDNS=0

ifndef CARBON_OBJECT_NAME_LENGTH
CARBON_OBJECT_NAME_LENGTH=16
endif

ifndef CARBON_LOGGER_BUFFER_LENGTH
CARBON_LOGGER_BUFFER_LENGTH=128
endif

ifndef CARBON_TIMER_COUNT
CARBON_TIMER_COUNT=2
endif

ifndef CARBON_EVENT_COUNT
CARBON_EVENT_COUNT=4
endif

ifndef CARBON_THREAD_COUNT
CARBON_THREAD_COUNT=2
endif

ifndef CARBON_THREAD_STACK_SIZE
CARBON_THREAD_STACK_SIZE=256
endif

ifndef CARBON_THREAD_PRIORITY
CARBON_THREAD_PRIORITY=6
endif
