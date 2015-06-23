
INCLUDE_PATH=-I$(TOPDIR)/include -I$(TOPDIR)/src/include -I./appProto/$(APP_PROTO_TYPE)  -I$(TOPDIR)/src/connecter/secure -I$(TOPDIR)/src/datatype 
#INCLUDE_PATH += 
LIB_PATH = $(TOPDIR)/lib

CFLAGS = $(INCLUDE_PATH)

ifeq ($(APP_PROTO_TYPE),coap)
ifeq ($(APP_SEC_TYPE),dtls)
CFLAGS += -DWILDDOG_PORT=5684
else
CFLAGS += -DWILDDOG_PORT=5683
endif
endif

LDFLAGS = 
#include_dirs= appProto

include $(TOPDIR)/env.mk
