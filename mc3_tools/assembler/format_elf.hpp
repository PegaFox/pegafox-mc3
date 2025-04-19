#include <cstdint>
#include <vector>

struct MemorySegment
{

};

std::vector<uint8_t> formatELF(std::vector<uint8_t> binary)
{
  binary.insert(binary.begin(), {
    0x7F, 'E', 'L', 'F', // magic number
    0x01, // 32 bit header format
    0x01, // little endian
    0x01, // header version
    0x00, // system V ABI
    0x00, // ABI flags
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8 padding bytes
    0x02, // executable object file
    0x00, // unsupported ISA
    0x01, // ELF version
    0x00, 0x00, 0x00, 0x00, // program entry point
    0x00, 0x00, 0x00, 0x00, // program header table pos
    0x00, 0x00, 0x00, 0x00, // section header table pos
    0x00, 0x00, 0x00, 0x00, // flags
    52, 0x00, // header size
    32, 0x00, // program header entry size
    0x00, 0x00, // program header table size
    40, 0x00, // section header entry size
    0x00, 0x00, // section header table size
    0x00, 0x00, // section header string table index
  });
  return binary;
}
