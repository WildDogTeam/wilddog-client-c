INCLUDE_PATH=-I$(TOPDIR)/include -I$(TOPDIR)/src -I $(TOPDIR)/../include
LIB_PATH=$(TOPDIR)/lib

CFLAGS= $(INCLUDE_PATH)
ifeq ($(PORT_TYPE), espressif)
INCLUDE_PATH+= -I ../../include -I $(TOPDIR)platform/espressif 
endif

ifneq ($(APP_SEC_TYPE), nosec)
CFLAGS+= -DWILDDOG_PORT=5684
else
CFLAGS+= -DWILDDOG_PORT=5683
endif

include $(TOPDIR)/project/espressif/env.mk

