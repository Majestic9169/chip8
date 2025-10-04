#ifndef EMU_H
#define EMU_H

#include "utils.h"
#include <SDL2/SDL_render.h>
#include <stdlib.h>

#define WIDTH 64
#define HEIGHT 32
#define WINDOW_WIDTH WIDTH * 10
#define WINDOW_HEIGHT HEIGHT * 10

// emu
int init_emu(char *rom_name);
void handle_input(char *rom_name);
void instructions();
void emu_run(char *rom_name);

// io
void clear_screen(SDL_Renderer *renderer);
void update_screen(SDL_Renderer *renderer);

#endif
