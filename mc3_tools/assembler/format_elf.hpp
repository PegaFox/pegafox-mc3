#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../elf_handler/elf.hpp"

#include <iostream>

void logElfInfo(ELF32& elf);

std::vector<uint8_t> formatELF(
  std::vector<uint8_t> binary,
  std::vector<ELF32::SymbolData> symbolTable)
{
  ELF32 result(0);

  Elf32_Addr pos = result.addSegment(PT_LOAD, 0, binary.size());
  std::copy(
    binary.begin(),
    binary.end(),
    result.data() + pos);

  result.addSymbolTable(symbolTable.data(), symbolTable.size());

  //logElfInfo(result);

  return std::vector<uint8_t>(result.data(), result.data() + result.size());
}

void logElfInfo(ELF32& elf)
{
  Elf32_Ehdr* header = elf.header();
  std::cout << "HEADER\n";

  std::cout << "SEGMENTS\n";
  for (uint8_t s = 0; Elf32_Phdr* pHeader = elf.programHeader(s); s++)
  {
    std::cout << "type: " << pHeader->p_type << '\n';
  }

  std::cout << "SECTIONS\n";
  for (uint8_t s = 0; Elf32_Shdr* sHeader = elf.sectionHeader(s); s++)
  {
    std::cout << "type: " << sHeader->sh_type << '\n';
    std::cout << "strPos: " << sHeader->sh_name << '\n';
    std::cout << "dataPos: " << sHeader->sh_offset << '\n';
  }
}
