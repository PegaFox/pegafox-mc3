#include <stddef.h>
#include <stdint.h>

void* memset(void* dest, int ch, size_t count)
{
  for (size_t i = 0; i < count; i++)
  {
    ((uint8_t*)dest)[i] = (uint8_t)ch;
  }

  return dest;
}
