# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project


# Add tropicssl include to all objects built for this target
INCLUDE_DIRS += .

# C source files included in this build.
CSRC += option.c
CSRC += pdu.c
CSRC += coap_debug.c
# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

