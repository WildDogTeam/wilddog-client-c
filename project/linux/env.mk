###########  MakeFile.env  ##########
# Top level pattern, include by Makefile of child directory
# in which variable like TOPDIR, TARGET or LIB may be needed

CC=gcc
MAKE=make

UNAR=ar x
RM = -rm -rf
MV = mv

CFLAGS+=-Wall -O2

ifeq ($(COVER), 1)
CFLAGS+= -fprofile-arcs -ftest-coverage
endif

ifeq ($(SELFTEST_TYPE), no)
CFLAGS += -DSELFTEST_TYPE=0
else
ifeq ($(SELFTEST_TYPE), ram)
CFLAGS += -DSELFTEST_TYPE=1
else
ifeq ($(SELFTEST_TYPE), time)
CFLAGS += -DSELFTEST_TYPE=2
else
ifeq ($(SELFTEST_TYPE), stab)
CFLAGS += -DSELFTEST_TYPE=3
else
ifeq ($(SELFTEST_TYPE), power)
CFLAGS += -DSELFTEST_TYPE=4
endif
endif
endif
endif
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
	test -d $(TOPDIR)/lib || mkdir -p $(TOPDIR)/lib
	test -d $(TOPDIR)/bin || mkdir -p $(TOPDIR)/bin

libs:$(OBJS)
ifneq ($(LIB), )
	$(UNAR) $(LIB)
	$(MV) *.o $(LIB_PATH)
endif

subdirs:$(SUBDIRS)
	for dir in $(SUBDIRS);\
		do $(MAKE) -C $$dir all||exit 1;\
	done

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(TOPDIR)/lib/*.o $(LDFLAGS)
	$(RM) *.d.* *.d
	$(RM) $(TOPDIR)/lib/*.o
	$(MV) $@ $(TOPDIR)/bin


$(OBJS):%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)
	$(RM) *.d.* *.d
	$(MV) $@ $(LIB_PATH)

-include $(DEPENDS)

$(DEPENDS):%.d:%.c
	set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[:]*,\1.o $@:,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$
	$(RM) $(DEPENDS)
	
clean:
	for dir in $(SUBDIRS);\
		do $(MAKE) -C $$dir clean||exit 1;\
	done
	$(RM) $(TARGET) $(LIB)  $(OBJS) $(DEPENDS) *.o
	$(RM) $(TOPDIR)/lib $(TOPDIR)/bin
	$(RM) *.d.*
