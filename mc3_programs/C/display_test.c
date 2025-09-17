#include <stdlib.h>
#include <stdbool.h>

const uint16_t width = 128;
const uint16_t height = 128;

int main()
{
  *(volatile uint16_t*)((&VGA)+1) = width; // width
  *(volatile uint16_t*)((&VGA)+3) = height; // height
  *((&VGA)+5) = 2; // byte depth
  VGA = 1; // update signal
  
  KEYBOARD_INSTRUCTION_OPCODE = 0xF4;
  while (KEYBOARD_TOP_SCANCODE != 0x29) {} // stall until space is pressed
  KEYBOARD_INSTRUCTION_OPCODE = 0xF5;

  VGA = 0; // switch to pixel input
  for (uint16_t p = 0; p < width*height; p++)
  {
    *(volatile uint16_t*)((&VGA)+1) = p; // pixel index
    *(volatile uint16_t*)((&VGA)+4) = p; // pixel color
  }

  TTY = 'D';
  TTY = 'o';
  TTY = 'n';
  TTY = 'e';
  TTY = '\n';

  while (true) {}

  return 0;
}

