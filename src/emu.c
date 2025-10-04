#include "../inc/emu.h"
#include "../inc/utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>

// === GLOBALS ===

// memory
uint8_t RAM[4096] = {0};
uint16_t stack[16] = {0};
// registers
uint8_t V[16] = {0};
uint16_t I = 0;
uint8_t delay = 0;
uint8_t sound = 0;
uint16_t pc = 0;
uint16_t *sp = stack;
// io
uint32_t display[64 * 32] = {0};
uint32_t keypad[16] = {0};
uint16_t opcode = 0;
int draw = 1;
int is_playing = 1;

// === EMU ===

// main game loop
void emu_run(char *rom_name) {
  // sdl init
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  clear_screen(renderer);

  // emu init
  init_emu(rom_name);

  // application loop
  while (is_playing) {
    for (int i = 0; i < 60; i++) {
      instructions();
    }
    SDL_Delay(1000 / 60);

    // update screen
    update_screen(renderer);

    // poll events
    handle_input(rom_name);

    // decrement timers
    if (delay > 0)
      delay--;
    if (sound > 0)
      sound--;
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

// initialise emulator state
int init_emu(char *rom_name) {
  load_font(RAM);

  // open rom
  FILE *rom = fopen(rom_name, "rb");
  if (!rom) {
    printf("ERROR: unable to open rom file %s\n", rom_name);
    exit(1);
  }

  // get file size (why is it this complicated)
  fseek(rom, 0, SEEK_END);
  const size_t rom_size = ftell(rom);
  printf("debug: ROM size is 0x%04lx B\n", rom_size);
  rewind(rom);

  fread(&RAM[0x200], rom_size, 1, rom);

  pc = 0x200;

  fclose(rom);

  return 1;
}

// handle user input
void handle_input(char *rom_name) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      is_playing = 0;
      exit(0);
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        is_playing = 0;
        exit(0);
      case SDLK_BACKSPACE:
        init_emu(rom_name);
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

// main cpu logic
void instructions() {
  opcode = RAM[pc] << 8 | RAM[pc + 1];
  uint16_t nnn = opcode & 0x0fff;
  uint8_t nn = opcode & 0x00ff;
  uint8_t n = opcode & 0x000f;
  uint8_t y = (opcode & 0x00f0) >> 4;
  uint8_t x = (opcode & 0x0f00) >> 8;

  printf("debug: [0x%04x] 0x%04x\t", pc, opcode);

  pc += 2;

  switch (opcode & 0xf000) {
  case 0x0000:
    switch (nn) {
    // RET: return from subroutine
    case 0xEE:
      printf("RET\n");
      pc = *(sp--);
      break;
    // CLS: clear screen
    case 0xE0:
      printf("CLS\n");
      for (int i = 0; i < WIDTH * HEIGHT; i++)
        display[i] = 0;
      draw = 1;
      break;
    }
    break;
  // JP nnn
  case 0x1000:
    printf("JP 0x%03x\n", nnn);
    pc = nnn;
    break;
  // CALL nnn
  case 0x2000:
    printf("CALL 0x%03x\n", nnn);
    *(++sp) = pc;
    pc = nnn;
    break;
  // SE Vx, nn
  case 0x3000:
    printf("SE Vx(0x%02x), 0x%02x\n", V[x], nn);
    if (V[x] == nn)
      pc += 2;
    break;
  // SNE Vx, nn
  case 0x4000:
    printf("SNE Vx(0x%02x), 0x%02x\n", V[x], nn);
    if (V[x] != nn)
      pc += 2;
    break;
  // SE Vx, Vy
  case 0x5000:
    printf("SNE Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
    if (V[x] == V[y])
      pc += 2;
    break;
  // LD Vx, nn
  case 0x6000:
    printf("LD Vx, 0x%02x\n", nn);
    V[x] = nn;
    break;
  // ADD Vx, nn
  case 0x7000:
    printf("ADD Vx(0x%02x), 0x%02x\n", V[x], nn);
    V[x] += nn;
    break;
  // ALU p much
  case 0x8000:
    switch (n) {
    // LD Vx, Vy
    case 0x0:
      printf("LD Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      V[x] = V[y];
      break;
    // OR Vx, Vy
    case 0x1:
      printf("OR Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      V[x] |= V[y];
      break;
    // AND Vx, Vy
    case 0x2:
      printf("AND Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      V[x] &= V[y];
      break;
    // XOR Vx, Vy
    case 0x3:
      printf("XOR Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      V[x] ^= V[y];
      break;
    // ADD Vx, Vy
    case 0x4: {
      printf("ADD Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      // TODO: understand the intricacies of this typecast
      int tmp = V[x] + V[y];
      V[x] += V[y];
      if (tmp > 255)
        V[0xF] = 1;
      else
        V[0xF] = 0;
    } break;
    // SUB Vx, Vy
    case 0x5:
      printf("SUB Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      if (V[x] > V[y]) {
        V[0xF] = 1;
      } else {
        V[0xF] = 0;
      }
      V[x] -= V[y];
      break;
    // SHR Vx
    case 0x6:
      printf("SHR Vx(0x%02x)\n", V[x]);
      V[0xF] = V[x] & 0x1;
      V[x] >>= 1;
      break;
    // SUBN Vx, Vy
    case 0x7:
      printf("SUBN Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
      if (V[y] > V[x])
        V[0xF] = 0;
      else
        V[0xF] = 1;
      V[x] = V[y] - V[x];
      break;
    // SHL Vx
    case 0xE:
      printf("SHL Vx(0x%02x)\n", V[x]);
      V[0xF] = (V[x] & 0x80) >> 7;
      V[x] <<= 1;
      break;
    }
    break;
  // SNE Vx, Vy
  case 0x9000:
    printf("SNE Vx(0x%02x), Vy(0x%02x)\n", V[x], V[y]);
    if (V[x] != V[y])
      pc += 2;
    break;
  // LD I, nnn
  case 0xA000:
    printf("LD I, 0x%03x\n", nnn);
    I = nnn;
    break;
  // JP V0, nnn
  case 0xB000:
    printf("JP V[0](0x%02x), 0x%03x\n", V[0], nnn);
    pc = nnn + V[0];
    break;
  // RND Vx, nn
  case 0xC000:
    printf("RND V[%x], 0x%02x\n", x, nn);
    V[x] = (rand() % 0x100) & nn;
    break;
  // DRW Vx, Vy, n
  case 0xD000:
    printf("DRW V[%x], V[%x], 0x%02x\n", x, y, nn);
    V[0xF] = 0;
    for (int i = 0; i < n; i++) {
      uint8_t pixel8 = RAM[I + i];
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
    break;
  case 0xE000:
    switch (nn) {
    // SKP Vx
    case 0x9E:
      printf("SKP V[%x]\n", x);
      if (keypad[V[x]] == 1) {
        pc += 2;
      }
      break;
    // SKNP Vx
    case 0xA1:
      printf("SKNP V[%x]\n", x);
      if (keypad[V[x]] == 0)
        pc += 2;
      break;
    }
    break;
  // timer stuff
  case 0xF000:
    switch (nn) {
    // LD Vx, DT
    case 0x07:
      printf("LD V[%x], DT(0x%02x)\n", x, delay);
      V[x] = delay;
      break;
    // LD Vx, K
    case 0x0A:
      for (int i = 0; i < 16; i++) {
        if (keypad[i] == 1) {
          printf("LD V[%x], K(%x)\n", x, i);
          V[x] = i;
          break;
        }
      }
      break;
    // LD DT, Vx
    case 0x15:
      printf("LD DT(0x%02x), V[%x]\n", delay, x);
      delay = V[x];
      break;
    // LD ST, Vx
    case 0x18:
      printf("LD ST(0x%02x), V[%x]\n", sound, x);
      sound = V[x];
      break;
    // ADD I, Vx
    case 0x1E:
      printf("ADD I(0x%02x), V[%x]\n", I, x);
      I += V[x];
      break;
    // LD F, Vx
    case 0x29:
      printf("LD F, Vx(%x)\n", V[x]);
      I = RAM[V[x] * 5];
      break;
    // LD B, Vx
    case 0x33:
      printf("LD B, Vx(%x)\n", V[x]);
      RAM[I + 2] = V[x] % 10;
      RAM[I + 1] = (V[x] % 100) / 10;
      RAM[I] = (V[x] / 100);
      break;
    // LD [I], Vx
    case 0x55:
      printf("LD [0x%04x], V[%x]\n", I, x);
      for (int i = 0; i <= x; i++) {
        RAM[I + i] = V[i];
      }
      break;
    // LD Vx, [I]
    case 0x65:
      printf("LD V[%x], [0x%04x]\n", x, I);
      for (int i = 0; i <= x; i++) {
        V[i] = RAM[I + i];
      }
      break;
    }
    break;
  }
}

// INPUT/OUTPUT

// clear the screen
void clear_screen(SDL_Renderer *renderer) {
  printf(" SCREEN IS BEING CLEARED\n");
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderClear(renderer);
}

// refresh the screen
void update_screen(SDL_Renderer *renderer) {
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
}
