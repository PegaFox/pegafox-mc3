pos 0x0000

setup_screen:
  set m2 0xFF08

  ~ initialize display to 255x255 with 1 byte per pixel
  set m0 0xFF
  put m0 2@m2+1
  put m0 2@m2+3

  set m0 0x01
  put m0 1@m2+5

  put m0 1@m2
  
set m0 0x00
put m0 1@m2

set m3 0xC0

set m1 draw_loop
draw_loop:
  put m0 2@m2+1
  put m3 1@m2+4
  inc m0
  set d0 m0
  rsh d0 8
  sub d0 0xFE
  jnz m1

~ stall
set m1 stall
stall:
  jnc m1

