
# Compiler flags
#CFLAGS =  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb 
#CFLAGS += $(patsubst %,-I$(SRC_PATH)/%,$(INCLUDE_DIRS)) -I.
#CFLAGS += -ffunction-sections -fmessage-length=0 -Wall

# Generate dependancy files automatically.
#CFLAGS += -MD -MP -MF $@.d
# C++ specific flags
#CPPFLAGS = -fno-exceptions -fno-rtti