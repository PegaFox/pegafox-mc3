#include <stdlib.h>
#include <stdbool.h>

const char* message = "Hello, World!\n";

void helloWorld()
{
  for (short int c = 0; message[c]; c++)
  {
    TTY = message[c];
  }
}

int main()
{
  while (true)
  {
    helloWorld();
  }

  return 0;
}
