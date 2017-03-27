
INCLUDE_PATH=-I$(TOPDIR)/include -I$(TOPDIR)/src
LIB_PATH = $(TOPDIR)/lib

CFLAGS = $(INCLUDE_PATH)

ifneq ($(APP_SEC_TYPE), nosec)
CFLAGS+= -DWILDDOG_PORT=5684
else
CFLAGS+= -DWILDDOG_PORT=5683
endif

LDFLAGS = 
ifeq ($(PORT_TYPE), espressif)
INCLUDE_PATH+= -I ../../include -I $(TOPDIR)platform/espressif 
endif

ifeq ($(PORT_TYPE), espressif)
INCLUDE_PATH += -I $(TOPDIR)/../include
endif
include $(TOPDIR)/project/$(PORT_TYPE)/env.mk
