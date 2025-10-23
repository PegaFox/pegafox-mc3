#include <stdlib.h>
#include <stdbool.h>

int main()
{
  HDD_SELECTED_BLOCK = 0;
  for (uint16_t p = 0; p < 256; p++)
  {
    HDD_SELECTED_WORD = p;
    HDD_RW_WORD = p;

    /*TTY = '0' + (p & 0xF);
    TTY = ',';
    TTY = ' ';
    TTY = '0' + (HDD_RW_WORD & 0xF);
    TTY = '\n';*/
  }

  TTY = 'D';
  TTY = 'o';
  TTY = 'n';
  TTY = 'e';
  TTY = '\n';

  //while (true) {}

  return 0;
}

