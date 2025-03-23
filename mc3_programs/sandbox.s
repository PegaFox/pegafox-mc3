pos 0x0000

setup_screen:
  set m2 0xFF08

  ~ initialize display to 255x255 with 1 byte per pixel
  put m0 1@m2+2
  put m0 1@m2+4

  set m0 0xFF
  put m0 1@m2+1
  put m0 1@m2+3

  set m0 0x01
  put m0 1@m2+5

  put m0 1@m2

  ~ draw a pixel in the middle
  set m0 0x00
  put m0 1@m2

  put m0 1@m2+2

  set m0 0x03
  put m0 1@m2+4


~ stall
set m1 stall
stall:
  jnc m1

setup_keyboard:
  ~ set m2 to 0xFF07
  set m2 0
  sub m2 0xF9

  set m0 0xF4
  put m0 1@m2+7

  keyboard_loop:
    set m0 1@m2+7
    put m0 1@m2
    set m1 keyboard_loop
    jnc m1

