all: main.c utils.h utils.c emu.h emu.c
	gcc -Wall -ggdb main.c utils.c utils.h emu.h emu.c -lSDL2 -lSDL2main -o chip8
