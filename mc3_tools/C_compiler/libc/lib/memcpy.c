#include <stddef.h>
#include <stdint.h>

void* memcpy(void* dest, const void* src, size_t count)
{
  for (size_t i = 0; i < count; i++)
  {
    ((uint8_t*)dest)[i] = ((const uint8_t*)src)[i];
  }

  return dest;
}
