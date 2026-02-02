
# PEGAFOX MC3 CPU

The successor to the MC1

## Register set

- 8 general purpose registers
  - 4 memory address registers
  - 4 data registers
- 1 program counter
- 1 interrupt vector register
  if interrupt vector is zero, interrupts are disabled
- 1 flag register(8 bits)
  - bits 0-3 - number of bits set in last alu operation
  - bit 4 - carry flag
  - bit 5 - overflow flag
  - bit 6 - zero flag
  - bit 7 - sign flag

## Instructions

All instructions are 16 bits.
Instructions are aligned at two byte boundaries.

### Instruction formats:

- opcode(5) + secondaryOpcode(11)
- opcode(5) + reg(3) + secondaryOpcode(8)
- opcode(5) + reg(3) + value(8)
- opcode(5) + reg(3) + reg(3) + reg(3) + valueBit(1) + useValue(1)
- opcode(5) + reg(3) + reg(2) + value(6)

0x00 OR reg(3) WITH value(8)
0x01 AND reg(3) WITH value(8)
0x02 XOR reg(3) WITH value(8)
//03 LSHIFT reg(3) BY value(8)
//04 RSHIFT reg(3) BY value(8)
0x03 ADD value(8) TO reg(3)
0x04 SUB value(8) FROM reg(3)
0x05 SINGLE_OPERAND
  0x00 NOT reg(3)
  0x01 READ flags TO reg(3)
  0x02 SET interruptVector TO reg(3)

0x06 SET reg(3) TO reg(3) ORED WITH reg(3) 00
0x06 SET reg(3) TO reg(3) ORED WITH value(4) 1
0x07 SET reg(3) TO reg(3) ANDED WITH reg(3) 00
0x07 SET reg(3) TO reg(3) ANDED WITH value(4) 1
0x08 SET reg(3) TO reg(3) XORED WITH reg(3) 00
0x08 SET reg(3) TO reg(3) XORED WITH value(4) 1
0x09 SET reg(3) TO reg(3) LSHIFTED BY reg(3) 00
0x09 SET reg(3) TO reg(3) LSHIFTED BY value(4) 1
0x0A SET reg(3) TO reg(3) RSHIFTED BY reg(3) 00
0x0A SET reg(3) TO reg(3) RSHIFTED BY value(4) 1
0x0B SET reg(3) TO reg(3) LROTATED BY reg(3) 00
0x0B SET reg(3) TO reg(3) LROTATED BY value(4) 1
0x0C SET reg(3) TO reg(3) RROTATED BY reg(3) 00
0x0C SET reg(3) TO reg(3) RROTATED BY value(4) 1
0x0D SET reg(3) TO reg(3) ADDED TO reg(3) 00
0x0D SET reg(3) TO reg(3) ADDED TO value(4) 1
0x0E SET reg(3) TO reg(3) SUBTRACTED WITH reg(3) 00
0x0E SET reg(3) TO reg(3) SUBTRACTED WITH value(4) 1

0x0F SET reg(3) TO value(8)

0x10 READ BYTE mem[reg(2)+signedValue(6)] TO reg(3)
0x11 READ WORD mem[reg(2)+signedValue(6)] TO reg(3)
0x12 WRITE BYTE mem[reg(2)+signedValue(6)] FROM reg(3)
0x13 WRITE WORD mem[reg(2)+signedValue(6)] FROM reg(3)

0x14 JUMP TO reg(3)+signedValue(8) IF (ZERO FLAG SET)
0x15 JUMP TO reg(3)+signedValue(8) IF (ZERO FLAG CLEAR)
0x16 JUMP TO reg(3)+signedValue(8) IF (CARRY FLAG SET)
0x17 JUMP TO reg(3)+signedValue(8) IF (CARRY FLAG CLEAR)
0x18 JUMP TO reg(3)+signedValue(8) IF (SIGN FLAG SET)
0x19 JUMP TO reg(3)+signedValue(8) IF (SIGN FLAG CLEAR)
0x1A JUMP TO reg(3)+signedValue(8) IF (OVERFLOW FLAG SET)
0x1B JUMP TO reg(3)+signedValue(8) IF (OVERFLOW FLAG CLEAR)
0x1C OPCODE_ONLY
  0x000 RETURN FROM INTERRUPT
0x1D
0x1E
0x1F

HALT if going from 0xFFFF to 0

## Interrupts and I/O

I/O devices can be memory mapped to any location.
I/O devices could also trigger interrupts.
Devices could trigger an interrupt pin, and send a 16 bit interrupt number. This number would be stored in a shift register acting as a queue of all interrupt requests. When the CPU is not running an interrupt, it will check the queue and if there is an interrupt request, it will run the interrupt routine, storing the current processor state in temporary registers.

Possible I/O devices:
- keyboard
- mouse
- tty
- disk
- network
- usb
- video card
- speakers
