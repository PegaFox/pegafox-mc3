pos 0x0000

setup_screen:
  set m2 0xFF08

  ~ initialize display to 16x16 with 1 byte per pixel
  put m0 1@m2+2
  put m0 1@m2+4

  set m0 0x10
  put m0 1@m2+1
  put m0 1@m2+3

  set m0 0x01
  put m0 1@m2+5

  put m0 1@m2
  
set m0 0x00
put m0 1@m2

set m1 draw_loop
draw_loop:
  put m0 1@m2+1
  put m0 1@m2+4
  inc m0
  and m0 0xFF
  jnz m1

~ stall
set m1 stall
stall:
  jnc m1

