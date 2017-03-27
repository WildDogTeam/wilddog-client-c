
INCLUDE_PATH=-I$(TOPDIR)/include -I$(TOPDIR)/src -I$(TOPDIR)/src/networking/$(APP_PROTO_TYPE)/ 

ifeq ($(PORT_TYPE), espressif)
INCLUDE_PATH+= -I ../../include -I $(TOPDIR)platform/espressif 
endif

LIB_PATH = $(TOPDIR)/lib

CFLAGS = $(INCLUDE_PATH)

LDFLAGS = 

include $(TOPDIR)/project/$(PORT_TYPE)/env.mk
