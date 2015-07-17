
# created by jimmy.pan

export TOPDIR=$(shell pwd)/../../

AR=ar cr

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


WD_OBJS := common platform

WD_OBJS_BUILD := $(patsubst %,%_build,$(WD_OBJS))
WD_OBJS_CLEAN := $(patsubst %,%_clean,$(WD_OBJS))

wdlib:$(WD_OBJS_BUILD)
	$(AR) libwilddog.a $(TOPDIR)/lib/*.o
	rm -rf $(TOPDIR)/lib/*.o
	mv libwilddog.a $(TOPDIR)/lib
	@echo "make wilddog lib end!"

common_build:
	make -C $(TOPDIR)/src all||exit 1;
	
common_clean:
	make -C $(TOPDIR)/src clean||exit 1;

platform_build:
	make -C $(TOPDIR)/platform all||exit 1;

platform_clean:
	make -C $(TOPDIR)/platform clean||exit 1;

test:
	make clean;make WILDDOG_SELFTEST=yes wdlib; \
	make WILDDOG_SELFTEST=yes -C $(TOPDIR)/tests/linux testall||exit 1;
	rm -rf $(TOPDIR)/lib/libwilddog.a
	@echo "make test end!"

example:
	if test ! -e $(TOPDIR)/lib/libwilddog.a ;\
	then \
		make wdlib; \
	fi
	make -C $(TOPDIR)/examples/linux demoall||exit 1;
	@echo "make example end!"

.PHONY: clean

clean:$(WD_OBJS_CLEAN)
	@echo "make clean end!"
	
