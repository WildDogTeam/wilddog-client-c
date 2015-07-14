
.PHONY: all test example clean cover

all:
	make -C ./project/linux wdlib||exit 1;

test:
	make -C ./project/linux test||exit 1;
	
example:
	make -C ./project/linux example||exit 1;
	
cover:clean
	make COVER=1 -C ./project/linux wdlib||exit 1;
	make COVER=1 -C ./project/linux test||exit 1;

clean:
	make -C ./project/linux clean||exit 1;

