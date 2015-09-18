
INCLUDE_PATH=-I$(TOPDIR)/include -I$(TOPDIR)/src
ifeq ($(PORT_TYPE), espressif)
INCLUDE_PATH += -I $(TOPDIR)/../include
endif
LIB_PATH = $(TOPDIR)/lib

CFLAGS = $(INCLUDE_PATH)
LDFLAGS = 
ifeq ($(PORT_TYPE), espressif)
INCLUDE_PATH+= -I ../../include -I $(TOPDIR)platform/espressif 
endif

include $(TOPDIR)/project/$(PORT_TYPE)/env.mk
