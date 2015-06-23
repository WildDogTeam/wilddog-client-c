
# created by jimmy.pan

export TOPDIR=$(shell pwd)

export TEST_TYPE = test


include client.config

ifndef PORT_TYPE
export PORT_TYPE=posix
endif

ifndef APP_PROTO_TYPE
export APP_PROTO_TYPE=coap
endif

ifndef APP_SEC_TYPE
export APP_SEC_TYPE=nosec
endif

ifndef DATA_TYPE
export DATA_TYPE=cbor
endif

include_dirs= src port
#exclude_dirs= include bin lib sample

wilddog:all wdlib

include $(TOPDIR)/env.mk

#CFLAGS = -I$(TOPDIR)/include -I$(TOPDIR)/src/include
#LDFLAGS = -L$(TOPDIR)/lib -lwilddog

wdlib:
	$(AR) libwilddog.a $(TOPDIR)/lib/*.o
	rm -rf $(TOPDIR)/lib/*.o
	mv libwilddog.a $(TOPDIR)/lib

example:
	cd  $(TOPDIR)/app/; make
	@echo "make example end!"
	
