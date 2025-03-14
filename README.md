PEGAFOX MC3 CPU // like MC1 but 16 bit

main addr modes
  reg
  reg+value
  value

8 general purpose registers
  4 memory address registers
  4 data registers
1 program counter
1 interrupt vector register
  if interrupt vector is zero, interrupts are disabled
1 flag register(8 bits)
  bits 0-3 - number of bits set in last alu operation
  bit 4 - carry flag
  bit 5 - overflow flag
  bit 6 - zero flag
  bit 7 - sign flag

jump instructions jump over two bytes at a time

all instructions are 16 bits
instructions are aligned at two byte boundaries

instruction formats:
  opcode(5) + secondaryOpcode(11)
  opcode(5) + reg(3) + secondaryOpcode(8)
  opcode(5) + reg(3) + value(8)
  opcode(5) + reg(3) + reg(3) + value(5)
  opcode(5) + reg(3) + reg(2) + value(6)

00000 OR reg(3) WITH value(8)
00001 AND reg(3) WITH value(8)
00010 XOR reg(3) WITH value(8)
00011 LSHIFT reg(3) BY value(8)
00100 RSHIFT reg(3) BY value(8)
00101 ADD value(8) TO reg(3)
00110 SUB value(8) FROM reg(3)
00111 SINGLE_OPERAND
  0x00 NOT reg(3)
  0x01 READ flags TO reg(3)
  0x02 SET interruptVector TO reg(3)

01000 OR reg(3) WITH reg(3)+signedValue(5)
01001 AND reg(3) WITH reg(3)+signedValue(5)
01010 XOR reg(3) WITH reg(3)+signedValue(5)
01011 LSHIFT reg(3) BY reg(3)+signedValue(5)
01100 RSHIFT reg(3) BY reg(3)+signedValue(5)
01101 ADD reg(3)+signedValue(5) TO reg(3)
01110 SUB reg(3)+signedValue(5) FROM reg(3)

01111 SET reg(3) to reg(3)+signedValue(5)
10000 SET reg(3) TO value(8)

10001 READ BYTE mem[reg(2)+signedValue(6)] TO reg(3)
10010 READ WORD mem[reg(2)+signedValue(6)] TO reg(3)
10011 WRITE BYTE mem[reg(2)+signedValue(6)] FROM reg(3)
10100 WRITE WORD mem[reg(2)+signedValue(6)] FROM reg(3)

10101 JUMP TO reg(3)+signedValue(8) IF (ZERO FLAG SET)
10110 JUMP TO reg(3)+signedValue(8) IF (ZERO FLAG CLEAR)
10111 JUMP TO reg(3)+signedValue(8) IF (CARRY FLAG SET)
11000 JUMP TO reg(3)+signedValue(8) IF (CARRY FLAG CLEAR)
11001 JUMP TO reg(3)+signedValue(8) IF (SIGN FLAG SET)
11010 JUMP TO reg(3)+signedValue(8) IF (SIGN FLAG CLEAR)
11011 JUMP TO reg(3)+signedValue(8) IF (OVERFLOW FLAG SET)
11100 JUMP TO reg(3)+signedValue(8) IF (OVERFLOW FLAG CLEAR)
11101 OPCODE_ONLY
  0x000 RETURN FROM INTERRUPT
11110
11111

HALT if going from 0xFFFF to 0

I/O devices can be memory mapped to any location
I/O devices could theoretically also trigger interrupts, but this is not implemented yet
Devices could trigger an interrupt pin, and send a 16 bit interrupt number. This number would be stored in a shift register acting as a queue of all interrupt requests. When the CPU is not running an interrupt, it will check the queue and if there is an interrupt request, it will run the interrupt routine, storing the current processor state in temporary registers

Possible I/O devices:
  keyboard
  mouse
  tty
  disk
  network
  usb
  video card
  speakers