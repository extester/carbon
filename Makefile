#
#   Carbon framework makefile
#
#   Copyright (c) 2010-2019 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 14.06.2010
#	Initial revision.
#
#   Revision 2.0, 26.02.2015
#	Rewrote build system
#
#   Revision 2.1, 27.09.2017 17:48:51
#	Added fake install target and distclean
#
#   make [RELEASE=1] [all|clean|rebuild|install]
#

DIRS = thparty backend src

include ./tool/multidir.mak

install:

distclean:
