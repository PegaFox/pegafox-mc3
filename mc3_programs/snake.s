pos 0x0000

~ setup m2 for io communication
set m2 0xFF00

setup_screen:
  ~ initialize display to 16x16 with 1 byte per pixel
  put m0 1@m2+10
  put m0 1@m2+12

  set m0 0x10
  put m0 1@m2+9
  put m0 1@m2+11

  set m0 0x01
  put m0 1@m2+13

  put m0 1@m2+8
  
  set m0 0x00
  put m0 1@m2+8

setup_keyboard:
  set m0 0xF4
  put m0 1@m2+14

main_loop:
  handle_input:
    set d2 move_snake
    set m0 1@m2+14

    jz d2

    set d0 key_released
    set d1 m0
    sub d1 0xF0
    jz d0

    set m1 snakeDir

    set d0 key_left
    set d1 m0
    sub d1 0x1C
    jz d0

    set d0 key_right
    set d1 m0
    sub d1 0x23
    jz d0
    
    set d0 key_up
    set d1 m0
    sub d1 0x1D
    jz d0
    
    set d0 key_down
    set d1 m0
    sub d1 0x1B
    jz d0

    jnz d2 
    
    key_left:
      set d1 0xFF
      put d1 1@m1
      jnz d2

    key_right:
      set d1 0x01
      put d1 1@m1
      jnz d2

    key_up:
      set d1 0xF0
      put d1 1@m1
      jnz d2

    key_down:
      set d1 0x10
      put d1 1@m1
      jnz d2
    
    key_released: ~ this must be at the end of input checking so it can lead to the next section
      set m0 1@m2+14

  move_snake:
    ~ retrieve snakeDir
    set m0 snakeDir
    set d0 1@m0

    ~ retrieve snakeEnd
    set m0 0x0100
    set m1 1@m0-1
    add m0 m1

    ~ retrieve snake head and update snakeEnd
    set d1 1@m0
    inc m0
    set m1 snakeEnd
    put m0 1@m1

    ~ calculate new head position
    add d1 d0
    put d1 1@m2+9
    set m3 0x10
    put m3 1@m2+12

    ~ get index and set new head position
    and m0 0xFF
    set m1 0x0100
    or m0 m1

    put d1 1@m0

    ~ if head is on food, skip removing the tail this loop
    set d2 skip_removing_tail
    and d1 0xFF
    set m0 foodPos
    set d0 1@m0
    sub d1 d0
    jz d2
    
    ~ remove tail
    set m0 snakeBegin
    set m1 1@m0
    
    add m1 0xFF
    inc m1
    set d0 1@m1
    put d0 1@m2+9
    set d0 0
    put d0 1@m2+12

    inc m1
    put m1 1@m0
  
    set m1 remove_tail_end
    jnz m1
    skip_removing_tail:

    ~ change foodPos to pseudorandom position
    set m0 snakeBegin
    set m1 1@m0
    set m0 1@m0+1
    xor d0 m0
    xor d0 m1
    ~set m1 1@m1
    ~set m0 1@m0
    ~xor d0 m0
    ~xor d0 m1

    set m0 foodPos
    put d0 1@m0
    remove_tail_end:

  draw_food:
    set m0 foodPos
    set m0 1@m0
    put m0 1@m2+9
    set m0 3
    put m0 1@m2+12

  draw_world:
    set d3 snake_draw_loop
    set m0 snakeEnd
    set d2 1@m0
    sub d2 3
    and d2 0xFF
    add d2 0xFF
    inc d2

    set m0 snakeBegin
    set m0 1@m0
    add m0 0xFF
    inc m0
    snake_draw_loop:
      ~set d0 1@m0
      ~put d0 1@m2+9

      ~set d0 0x2
      ~put d0 1@m2+12

      set d0 0x0100

      inc m0
      and m0 0xFF
      or m0 d0
      set d1 m0
      
      sub d1 d2
      jnz d3

  set d0 main_loop
  jnz d0

~ stall
set m1 stall
stall:
  jnc m1

var foodPos[1]

var snakeDir[1] = 0x01

var snakeBegin[1] @0x00FE = 0x00
var snakeEnd[1] @0x00FF = 0x04
var snake[256] @0x0100


