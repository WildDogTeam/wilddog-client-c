
INCLUDE_PATH=-I$(TOPDIR)/include

LIB_PATH = $(TOPDIR)/lib
#TARGET defined if main in this dir
#TARGET:=demo
CFLAGS = $(INCLUDE_PATH)
LDFLAGS := -L$(TOPDIR)/lib -lwilddog


DEMO_SRCS=$(wildcard *.c)
DEMO_OBJS=$(patsubst %.c, %, $(DEMO_SRCS))

demoall:
	@for demo_obj in $(DEMO_OBJS);\
		do\
			$(CC) $(CFLAGS) -o $$demo_obj $$demo_obj.c $(LDFLAGS);\
			mv $$demo_obj $(TOPDIR)/bin;\
			rm -rf *.d.* *.d;\
		done


include $(TOPDIR)/project/linux/env.mk
