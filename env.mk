###########  MakeFile.env  ##########
# Top level pattern, include by Makefile of child directory
# in which variable like TOPDIR, TARGET or LIB may be needed

CC=gcc
MAKE=make

AR=ar cr
UNAR=ar x
RM = -rm -rf

CFLAGS+=-Wall -O2

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
#$(TARGET)

prepare:
	test -d $(TOPDIR)/lib || mkdir -p $(TOPDIR)/lib
	test -d $(TOPDIR)/bin || mkdir -p $(TOPDIR)/bin

libs:$(OBJS)
ifneq ($(LIB), )
	$(UNAR) $(LIB)
	mv *.o $(LIB_PATH)
endif

subdirs:$(SUBDIRS)
	@echo subdirs
	for dir in $(SUBDIRS);\
		do $(MAKE) -C $$dir all||exit 1;\
	done

$(TARGET):$(OBJS)
	$(CC) -o $@ $(TOPDIR)/lib/$^ $(LDFLAGS)
	$(RM) $@.d.* $@.d
	mv $@ $(TOPDIR)/bin


$(OBJS):%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)
	mv $@ $(LIB_PATH) 


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
	rm -rf $(TOPDIR)/lib $(TOPDIR)/bin
	rm -rf *.d.*
