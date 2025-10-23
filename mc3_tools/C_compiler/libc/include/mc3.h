#ifndef _MC3_H
#define _MC3_H 1

  #include <stdint.h>

// uses seven bytes
// read/write bytes 1-4: access the selected block.
// read/write byte 5: access the selected word.
// read/write bytes 6-7: access the word at the specified offset in the selected block.
  #define HDD (*(volatile uint8_t*)(0xFF00))
  #define HDD_SELECTED_BLOCK (*(volatile uint32_t*)(0xFF00))
  #define HDD_SELECTED_WORD (*(volatile uint8_t*)(0xFF04))
  #define HDD_RW_WORD (*(volatile uint16_t*)(0xFF05))

  #define TTY (*(volatile uint8_t*)(0xFF07))

  #define VGA (*(volatile uint8_t*)(0xFF08))
  #define VGA_WRITE_SCREEN_WIDTH (*(volatile uint16_t*)(0xFF09))
  #define VGA_WRITE_SCREEN_HEIGHT (*(volatile uint16_t*)(0xFF0B))
  #define VGA_WRITE_BYTES_PER_PIXEL (*(volatile uint8_t*)(0xFF0D))
  #define VGA_PUSH_COMMAND (*(volatile uint8_t*)(0xFF08))
  #define VGA_VRAM_PTR_24_BIT (*(volatile uint8_t*)(0xFF09))
  #define VGA_VRAM_POINTED_DATA (*(volatile uint16_t*)(0xFF0C))

  #define KEYBOARD (*(volatile uint8_t*)(0xFF0E))
  #define KEYBOARD_TOP_SCANCODE (*(volatile uint8_t*)(0xFF0E))
  #define KEYBOARD_INSTRUCTION_OPCODE (*(volatile uint8_t*)(0xFF0E))
  #define KEYBOARD_INSTRUCTION_RESPONSE (*(volatile uint8_t*)(0xFF0F))
  #define KEYBOARD_INSTRUCTION_OPERAND (*(volatile uint8_t*)(0xFF0F))

  #define MOUSE (*(volatile uint8_t*)(0xFF10))

  #define SPEAKER (*(volatile uint8_t*)(0xFF14))

  #endif /* _MC3_H */
