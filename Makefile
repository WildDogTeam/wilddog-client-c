
MAKE=make --no-print-directory

.PHONY: all test example clean cover

all:
	@echo "Building wilddog lib"
	@$(MAKE) -C ./project/linux wdlib||exit 1;
	@echo "Building wilddog lib end!"

test:
	@echo "Building test"
	@$(MAKE) -C ./project/linux test||exit 1;
	@echo "Building test end!"

example:
	@echo "Building example"
	@$(MAKE) -C ./project/linux example||exit 1;
	@echo "Build example end!"
	
cover:clean
	@echo "Building cover check"
	@$(MAKE) COVER=1 -C ./project/linux test||exit 1;
	@echo "Build cover end!"
clean:
	@echo "Cleaning"
	@$(MAKE) -C ./project/linux clean||exit 1;
	@echo "Clean end!"

%::
	@echo "\nUsage: make [option] [VERBOSE=1]\n"
	@echo "option:\n"
	@echo "\tall\t build wilddog lib by default\n"
	@echo "\ttest\t build executables for test\n"
	@echo "\texample\t build executables examples\n"
	@echo "\tcover\t build cover test executable files(must support lib lcov)\n"
	@echo "\tclean\t clean temporary files\n"
	@echo "VERBOSE:\n\t when set to 1, output all logs\n"
	