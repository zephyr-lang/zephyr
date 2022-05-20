.PHONY: default all clean bootstrap bootstrapcmp

default: 
	./build/zephyr -o ./build/zephyr ./src/main.zpr
all: default

bootstrap: bootstrapasm bootstrapcmp

bootstrapasm:
	yasm -felf64 -o ./bootstrap/bootstrap_x86_64_linux.o ./bootstrap/bootstrap_x86_64_linux.yasm
	ld -o ./bootstrap/bootstrap_x86_64_linux ./bootstrap/bootstrap_x86_64_linux.o

bootstrapcmp:
	./bootstrap/bootstrap_x86_64_linux -o ./build/zephyr ./src/main.zpr

clean:
	-rm -f build/*.o
	-rm -f bin/*.o
	-rm -f build/*.yasm

clean-all: clean
	-rm -f build/*