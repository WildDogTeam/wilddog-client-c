# Define the compiler/tools prefix
#GCC_PREFIX = arm-none-eabi-
GCC_PREFIX =
PORT_PATH= ../../ports/posix
# Define tools
CC = $(GCC_PREFIX)gcc
AR = $(GCC_PREFIX)ar
OBJCOPY = $(GCC_PREFIX)objcopy

RM = rm -f
MKDIR = mkdir -p

# Recursive wildcard function
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))


BUILD_PATH = ./build

SRC_PATH = ../../


# Target this makefile is building.
TARGET = $(BUILD_PATH)/libwilddog.a

# Find all build.mk makefiles in each source directory in the src tree.
SRC_MAKEFILES := $(call rwildcard,$(SRC_PATH)/src,build.mk)
SRC_MAKEFILES += $(PORT_PATH)/build.mk

all: check_external_deps $(TARGET)
# Include all build.mk defines source files.
include $(SRC_MAKEFILES)

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(ASRC:.S=.o))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(ASRC:.S=.o.d))

CFLAGS += $(patsubst %,-I$(SRC_PATH)/%,$(INCLUDE_DIRS)) -I.
CFLAGS += -MD -MP -MF $@.d
# All Target

# Check for external dependancies, none in this case.
check_external_deps:
	@echo ' '
# Tool invocations
$(TARGET) : $(ALLOBJ)
	
	$(AR) -r $@ $^
	@echo ' '

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(SRC_PATH)/%.c
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(SRC_PATH)/%.cpp
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	@echo

# Other Targets
clean:
	$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGET)
	@echo ' '

ports:
	cp -r src/ ports/wiced/app/wilddog/skel/

#make samples
SAMPLE=wilddog_linux_client

$(SAMPLE) : $(SAMPLE).c $(TARGET) 

	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SAMPLE).c -L./build -lwilddog -lm
	#mv $(SAMPLE) ./
	

sample: $(SAMPLE) 
	@echo "making samples"	


.PHONY: all clean sample ports
.SECONDARY:

# Include auto generated dependancy files
-include $(ALLDEPS)
