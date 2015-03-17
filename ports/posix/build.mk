
PORT_DIR=ports/posix/
INCLUDE_DIRS += $(PORT_DIR)
CSRC += $(PORT_DIR)posix.c
CPPSRC +=
ASRC +=

#make samples
SAMPLE=$(PORT_DIR)sample/wilddog_linux_client

$(SAMPLE) : $(SAMPLE).c $(TARGET) 

	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SAMPLE).c -L./build -lwilddog -lm
	mv $(SAMPLE) ./
	

sample: $(SAMPLE) 
	@echo "making samples"	
