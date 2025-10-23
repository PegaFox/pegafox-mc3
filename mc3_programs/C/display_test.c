#include <stdlib.h>
#include <stdbool.h>

const uint16_t width = 128;
const uint16_t height = 128;

int main()
{
  VGA_WRITE_SCREEN_WIDTH = width; // width
  VGA_WRITE_SCREEN_HEIGHT = height; // height
  VGA_WRITE_BYTES_PER_PIXEL = 1; // byte depth
  VGA_PUSH_COMMAND = 1; // update signal
  
  KEYBOARD_INSTRUCTION_OPCODE = 0xF4;
  while (KEYBOARD_TOP_SCANCODE != 0x29) {} // stall until space is pressed
  KEYBOARD_INSTRUCTION_OPCODE = 0xF5;

  VGA = 0; // switch to pixel input
  for (uint16_t p = 0; p < (width*height); p += 1)
  {
    *(volatile uint16_t*)(&VGA_VRAM_PTR_24_BIT) = p; // pixel index
    *(volatile uint8_t*)(&VGA_VRAM_POINTED_DATA) = p; // pixel color
    //*(volatile uint16_t*)(&VGA_RAM_PTR_24_BIT) = p+1; // pixel index
    //VGA_VRAM_POINTED_DATA:= (p+1) | 0x80; // pixel color
  }

  TTY = 'D';
  TTY = 'o';
  TTY = 'n';
  TTY = 'e';
  TTY = '\n';

  while (true) {}

  return 0;
}
