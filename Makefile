CC:=gcc
default: always bin/shell
always:
	mkdir -p bin
bin/shell: src/main.c
	gcc $^ -o $@
clean:
	rm -rf bin
