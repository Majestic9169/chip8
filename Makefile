INC_FILES := $(wildcard inc/*.h)
SRC_FILES := $(wildcard src/*.c)

chip8: main.c ${INC_FILES} ${SRC_FILES}
	gcc -Wall -ggdb -lSDL2 -lSDL2main -o chip8 $^

run: 
	./chip8 ./roms/hbd.ch8

clean:
	rm chip8
