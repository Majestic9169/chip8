#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Macros
#define WINDOW_WIDTH 64 * 10
#define WINDOW_HEIGHT 32 * 10

// global variables
// chip8 mem
uint8_t ram[4096] = {0};
uint8_t V[16] = {0};
uint16_t I = 0;
uint8_t delay = 0;
uint8_t sound = 0;
uint16_t pc = 0;
uint16_t stack[16] = {0};
uint16_t *sp = stack;
uint32_t display[64 * 32] = {0};
uint32_t keypad[16] = {0};
uint16_t opcode = 0;
int draw = 1;
int playing = 1;

int x, y, n, nn, nnn = 0;

const char *rom_name;

int init_emu() {
  pc = 0x200;
  const char font[] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  for (int i = 0; i < sizeof(font); i++) {
    ram[i] = font[i];
  }

  printf(" FONT HAS BEEN LOADED TILL ADDRESS %lu (dec)\n", sizeof(font));

  // open rom
  FILE *rom = fopen(rom_name, "rb");
  if (!rom) {
    printf(" ROM File is invalid\n");
  }

  // get file size (why is it this complicated)
  fseek(rom, 0, SEEK_END);
  const size_t rom_size = ftell(rom);
  printf(" ROM SIZE IS %lu B\n", rom_size);
  rewind(rom);

  fread(&ram[0x200], rom_size, 1, rom);

  fclose(rom);

  for (int i = 1; i < 10; i++) {
    printf("  ram[0x%X] = 0x%X\n", 0x1FF + i, ram[0x1FF + i]);
  }

  return 1;
}

void clear_screen(SDL_Renderer *renderer) {
  printf(" SCREEN IS BEING CLEARED\n");
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderClear(renderer);
}

void update_screen(SDL_Renderer *renderer) {
  SDL_Rect pixel = {.x = 0, .y = 0, .w = 1, .h = 1};

  for (int i = 0; i < sizeof(display); i++) {
    pixel.x = (i % 600);
    pixel.y = (i / 600);

    if (display[i]) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderFillRect(renderer, &pixel);
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      SDL_RenderFillRect(renderer, &pixel);
    }
  }

  SDL_RenderPresent(renderer);
}

void handle_input() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      playing = 0;
      return;
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        playing = 0;
        return;
      case SDLK_BACKSPACE:
        init_emu();
        break;

      case SDLK_1:
        keypad[0x1] = 1;
        break;

      case SDLK_2:
        keypad[0x2] = 1;
        break;

      case SDLK_3:
        keypad[0x3] = 1;
        break;

      case SDLK_4:
        keypad[0xC] = 1;
        break;

      case SDLK_q:
        keypad[0x4] = 1;
        break;

      case SDLK_w:
        keypad[0x5] = 1;
        break;

      case SDLK_e:
        keypad[0x6] = 1;
        break;

      case SDLK_r:
        keypad[0xD] = 1;
        break;

      case SDLK_a:
        keypad[0x7] = 1;
        break;

      case SDLK_s:
        keypad[0x8] = 1;
        break;

      case SDLK_d:
        keypad[0x9] = 1;
        break;

      case SDLK_f:
        keypad[0xE] = 1;
        break;

      case SDLK_z:
        keypad[0xA] = 1;
        break;

      case SDLK_x:
        keypad[0x0] = 1;
        break;

      case SDLK_c:
        keypad[0xB] = 1;
        break;

      case SDLK_v:
        keypad[0xF] = 1;
        break;

      default:
        break;
      }
      break;

    case SDL_KEYUP:
      switch (event.key.keysym.sym) {

      case SDLK_1:
        keypad[0x1] = 0;
        break;

      case SDLK_2:
        keypad[0x2] = 0;
        break;

      case SDLK_3:
        keypad[0x3] = 0;
        break;

      case SDLK_4:
        keypad[0xC] = 0;
        break;

      case SDLK_q:
        keypad[0x4] = 0;
        break;

      case SDLK_w:
        keypad[0x5] = 0;
        break;

      case SDLK_e:
        keypad[0x6] = 0;
        break;

      case SDLK_r:
        keypad[0xD] = 0;
        break;

      case SDLK_a:
        keypad[0x7] = 0;
        break;

      case SDLK_s:
        keypad[0x8] = 0;
        break;

      case SDLK_d:
        keypad[0x9] = 0;
        break;

      case SDLK_f:
        keypad[0xE] = 0;
        break;

      case SDLK_z:
        keypad[0xA] = 0;
        break;

      case SDLK_x:
        keypad[0x0] = 0;
        break;

      case SDLK_c:
        keypad[0xB] = 0;
        break;

      case SDLK_v:
        keypad[0xF] = 0;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }
  }
}

void instructions() {
  opcode = ram[pc] << 8 | ram[pc + 1];
  printf(" 0x%X   0x%X\n", pc, opcode);
  switch (opcode & 0xF000) {
  case 0x0000:
    switch (opcode & 0x000F) {
    case 0x000E:
      printf("  RET\n");
      pc = *(sp--);
      printf("   SP = %X\n", *sp);
      break;
    case 0x0000:
      printf("  CLS\n");
      for (int i = 0; i < 64 * 32; i++)
        display[i] = 0;
      pc += 2;
      draw = 1;
      break;
    }
    break;
  case 0x1000:
    pc = (opcode << 4 & 0x0FFF0) >> 4;
    printf("  JP addr to 0x%X\n", (opcode << 4 & 0x0FFF0) >> 4);
    break;
  case 0x2000:
    *(++sp) = pc + 2;
    pc = opcode & 0x0FFF;
    printf("  CALL addr 0x%X\n", pc);
    printf("   SP = %X\n", *sp);
    break;
  case 0x3000:
    x = (opcode & 0x0F00) >> 8;
    nn = opcode & 0x00FF;
    if (V[x] == nn)
      pc += 2;
    pc += 2;
    break;
  case 0x4000:
    x = (opcode & 0x0F00) >> 8;
    nn = opcode & 0x00FF;
    if (V[x] != nn)
      pc += 2;
    pc += 2;
    break;
  case 0x5000:
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    pc += 2;
    if (V[x] == V[y])
      pc += 2;
    break;
  case 0x6000:
    nn = opcode & 0x00FF;
    x = (opcode & 0x0F00) >> 8;
    V[x] = nn;
    pc += 2;
    break;
  case 0x7000:
    nn = opcode & 0x00FF;
    x = (opcode & 0x0F00) >> 8;
    V[x] += nn;
    pc += 2;
    break;
  case 0x8000:
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    switch (opcode & 0x000F) {
    case 0x0000:
      V[x] = V[y];
      pc += 2;
      break;
    case 0x0001:
      V[x] |= V[y];
      pc += 2;
      break;
    case 0x0002:
      V[x] &= V[y];
      pc += 2;
      break;
    case 0x0003:
      V[x] ^= V[y];
      pc += 2;
      break;
    case 0x0004:
      if (V[y] + V[x] > 255)
        V[0xF] = 1;
      else
        V[0xF] = 0;
      V[x] += V[y];
      V[x] &= 0xFF;
      pc += 2;
      break;
    case 0x0005:
      if (V[x] > V[y]) {
        V[0xF] = 1;
      } else {
        V[0xF] = 0;
      }
      V[x] -= V[y];
      pc += 2;
      break;
    case 0x0006:
      V[0xF] = V[x] & 0x1;
      V[x] >>= 1;
      pc += 2;
      break;
    case 0x0007:
      pc += 2;
      if (V[x] > V[y])
        V[0xF] = 0;
      else
        V[0xF] = 1;
      V[x] = V[y] - V[x];
      break;
    case 0x000E:
      V[0xF] = (V[x] & 0x80) >> 7;
      V[x] <<= 1;
      V[x] &= 0x0FF;
      pc += 2;
      break;
    }
    break;
  case 0x9000:
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    pc += 2;
    if (V[x] != V[y])
      pc += 2;
    break;
  case 0xA000:
    nnn = opcode & 0x0FFF;
    I = nnn;
    pc += 2;
    break;
  case 0xB000:
    nnn = opcode & 0x0FFF;
    pc = nnn + V[0];
    break;
  case 0xC000:
    x = (opcode & 0x0F00 >> 8);
    nn = opcode & 0x00FF;
    srand(time(0));
    V[x] = (rand() % 256) & nn;
    pc += 2;
    break;
  case 0xD000:
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    n = opcode & 0x000F;

    V[0xF] = 0;
    for (int i = 0; i < n; i++) {
      uint8_t pixel8 = ram[I + i];
      for (int j = 0; j < 8; j++) {
        // coordinate Vx, Vy means byte display[V[y] * 64 + V[x]]
        // one byte is smthg like 10101101 which means
        if ((display[(V[y] + i) * 64 + V[x] + j] == 1) &&
            (pixel8 >> (7 - j) == 1))
          V[0xF] = 1;
        display[(V[y] + i) * 64 + V[x] + j] ^= (pixel8 >> (7 - j)) & 1;
      }
    }

    draw = 1;
    pc += 2;
    break;
  case 0xE000:
    switch (opcode & 0x000F) {
    case 0x000E:
      x = (opcode & 0x0F00) >> 8;
      if (keypad[V[x]] == 1) {
        pc += 2;
      }
      pc += 2;
      break;
    case 0x0001:
      x = (opcode & 0x0F00) >> 8;
      if (keypad[V[x]] == 0)
        pc += 2;
      pc += 2;
      break;
    }
  case 0xF000:
    switch (opcode & 0x00FF) {
    case 0x0007:
      x = (opcode & 0x0F00) >> 8;
      V[x] = delay;
      pc += 2;
      break;
    case 0x000A:
      x = (opcode & 0x0F00) >> 8;
      for (int i = 0; i < 16; i++) {
        if (keypad[i] == 1) {
          V[x] = i;
          pc += 2;
          break;
        }
      }
      break;
    case 0x0015:
      x = (opcode & 0x0F00) >> 8;
      delay = V[x];
      pc += 2;
      break;
    case 0x0018:
      x = (opcode & 0x0F00) >> 8;
      sound = V[x];
      pc += 2;
      break;
    case 0x001E:
      x = (opcode & 0x0F00) >> 8;
      I += V[x];
      pc += 2;
      break;
    case 0x0029:
      x = (opcode & 0x0F00) >> 8;
      I = ram[V[x] * 5];
      pc += 2;
      break;
    case 0x0033:
      x = (opcode & 0x0F00) >> 8;
      ram[I + 2] = V[x] % 10;
      ram[I + 1] = (V[x] % 100) / 10;
      ram[I] = (V[x] / 100);
      pc += 2;
      break;
    case 0x0055:
      x = (opcode & 0x0F00) >> 8;
      for (int i = 0; i <= x; i++) {
        ram[I + i] = V[i];
      }
      pc += 2;
      break;
    case 0x0065:
      x = (opcode & 0x0F00) >> 8;
      for (int i = 0; i <= x; i++) {
        V[i] = ram[I + i];
      }
      pc += 2;
      break;
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf(" NO ROM PASSED\n");
    exit(EXIT_FAILURE);
  }

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  rom_name = argv[1];
  printf(" ROM NAME is %s\n", rom_name);

  init_emu();

  clear_screen(renderer);
  /* SDL_RenderPresent(renderer); */

  while (playing) {

    for (int i = 0; i < 7000; i++) {
      instructions();
      handle_input();
    }

    if (draw == 1) {
      for (int i = 0; i < 64 * 32; i++) {
        SDL_Rect pixel = {
            .x = 0, .y = 0, .w = WINDOW_WIDTH / 64, .h = WINDOW_HEIGHT / 32};
        pixel.x = i % WINDOW_WIDTH;
        pixel.y = i / WINDOW_WIDTH;
        if (display[i]) {
          SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
          SDL_RenderFillRect(renderer, &pixel);
        } else {
          SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
          SDL_RenderFillRect(renderer, &pixel);
        }
      }
      SDL_RenderPresent(renderer);
      draw = 0;
    }

    if (delay > 0)
      delay--;
    if (sound > 0)
      sound--;
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 1;
}
