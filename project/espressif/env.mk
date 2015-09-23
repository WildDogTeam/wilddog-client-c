###########  MakeFile.env  ##########
# Top level pattern, include by Makefile of child directory
# in which variable like TOPDIR, TARGET or LIB may be needed

AR := xtensa-lx106-elf-ar
CC = xtensa-lx106-elf-gcc
NM = xtensa-lx106-elf-nm
CPP = xtensa-lx106-elf-cpp
OBJCOPY = xtensa-lx106-elf-objcopy
OBJDUMP = xtensa-lx106-elf-objdump

MAKE=make

UNAR=ar x
RM = rm -rf
MV = mv

ifeq ($(VERBOSE),1)
QUIET = 
else
QUIET = @
MAKE += --no-print-directory
endif
CFLAGS+= \
	-g			\
	-O2			\
	-Wpointer-arith		\
	-Wundef			\
	-Werror			\
	-Wl,-EL			\
	-fno-inline-functions	\
	-nostdlib       \
	-mlongcalls	\
	-mtext-section-literals \
	-ffunction-sections \
	-fdata-sections
#	-Wall			


CFLAGS+= -DWILDDOG_PORT_TYPE_ESP
CFLAGS+= -DICACHE_FLASH
#CFLAGS+= -DWILDDOG_SELFTEST


ifeq ($(COVER), 1)
CFLAGS+= -fprofile-arcs -ftest-coverage
endif

ifeq ($(WILDDOG_SELFTEST), yes)
CFLAGS+= -DWILDDOG_SELFTEST
endif


ifeq ($(include_dirs), )
dirs:=$(shell find . -maxdepth 1 -type d)
dirs:=$(basename $(patsubst ./%,%,$(dirs)))
dirs:=$(filter-out $(exclude_dirs),$(dirs))
SUBDIRS := $(dirs)
else
SUBDIRS := $(include_dirs)
endif
SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
DEPENDS=$(SRCS:%.c=%.d)


all: prepare $(TARGET) libs subdirs 

prepare:
	$(QUIET)test -d $(TOPDIR)/lib || mkdir -p $(TOPDIR)/lib; \
	test -d $(TOPDIR)/bin || mkdir -p $(TOPDIR)/bin

libs:$(OBJS)
ifneq ($(LIB), )
	$(QUIET)$(UNAR) $(LIB); \
	$(MV) *.o $(LIB_PATH)
endif

subdirs:$(SUBDIRS)
	$(QUIET)for dir in $(SUBDIRS); do \
		echo "Building" $$dir; \
		$(MAKE) -C $$dir all||exit 1;\
	done

$(TARGET):$(OBJS)
	$(QUIET)$(CC) $(CFLAGS) -o $@ $(TOPDIR)/lib/*.o $(LDFLAGS); 
	$(RM) *.d.* *.d ; \
	$(RM) $(TOPDIR)/lib/*.o; \
	$(MV) $@ $(TOPDIR)/bin; \

$(OBJS):%.o:%.c
	$(QUIET)$(CC) -c $< -o $@ $(CFLAGS); \
	$(RM) *.d.* *.d; \
	$(MV) $@ $(LIB_PATH)

-include $(DEPENDS)

$(DEPENDS):%.d:%.c
	$(QUIET)set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[:]*,\1.o $@:,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$ ; \
	$(RM) $(DEPENDS)
	
clean:
	$(QUIET)for dir in $(SUBDIRS);\
		do $(MAKE) -C $$dir clean||exit 1;\
	done
	$(QUIET)$(RM) $(TARGET) $(LIB)  $(OBJS) $(DEPENDS) *.o; \
	$(RM) $(TOPDIR)/lib $(TOPDIR)/bin; \
	$(RM) *.d.*
