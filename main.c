#include "inc/emu.h"
#include <time.h>

int main(int argc, char **argv) {
  srand(time(0));

  if (argc < 2) {
    printf("usage: ./chip8 <rom_file>\n");
    exit(1);
  }

  emu_run(argv[1]);

  return 1;
}
