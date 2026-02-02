#ifndef MC3_ELF_HANDLER_ELF_HPP
#define MC3_ELF_HANDLER_ELF_HPP

#include <map>
#include <string>
#include <vector>
#include <elf.h>

// Segments and sections can overlap
class ELF32
{
  public:
    static_assert(sizeof(Elf32_Ehdr) == 52);
    static_assert(sizeof(Elf32_Phdr) == 32);
    static_assert(sizeof(Elf32_Shdr) == 40);

    // This initializes a basic elf file
    ELF32(uint16_t entryPoint);

    // This loads an existing elf file
    ELF32(const uint8_t* begin, const uint8_t* end);

    // Type can be any PT_* value as defined in elf.h
    Elf32_Addr addSegment(Elf32_Word type, Elf32_Addr ramPos, Elf32_Addr size);

    // Type can be any SHT_* value as defined in elf.h
    Elf32_Addr addSection(
      Elf32_Word type,
      Elf32_Addr ramPos,
      Elf32_Addr size,
      const std::string& name = "");

    struct SymbolData
    {
      std::string name;
      Elf32_Addr value;
      Elf32_Word size;
      uint8_t type;
    };

    void addSymbolTable(SymbolData* symbols, Elf32_Word count);

    struct SymbolTable
    {
      Elf32_Addr strings;
      Elf32_Addr symbols;
      Elf32_Half symbolCount;
    };

    SymbolTable getSymbolTable(Elf32_Half index);

    bool valid();

    Elf32_Ehdr* header();

    Elf32_Phdr* programHeader(Elf32_Half index);

    // Section 0 is always null
    // Section 1 is a string table
    // The other sections can be user defined
    Elf32_Shdr* sectionHeader(Elf32_Half index);

    uint8_t* data();

    // Returns the size in bytes of this->data()
    uint32_t size();

    template <typename IIt>
    Elf32_Addr insert(Elf32_Addr pos, IIt begin, IIt end);

    template <typename T>
    Elf32_Addr insert(Elf32_Addr pos, T data);

  private:
    static const uint8_t ident[EI_NIDENT];

    std::vector<uint8_t> file;

    Elf32_Ehdr initHeader(uint16_t entryPoint);

    Elf32_Addr addString(std::string string);

    enum AllocationType
    {
      Header,
      SegmentHeader,
      SectionHeader,
      Filedata,
    };

    // This should be run after inserting or removing something from the center of the file
    void syncPointers(Elf32_Addr pos, Elf32_Off len, AllocationType type);

    // Returns whether pointer was changed
    bool syncPointer(Elf32_Addr pos, Elf32_Off len, Elf32_Addr& ptr, bool append);
};

struct SymbolData
{
  enum
  {
    Label,
    Variable,
  } type;
  std::string name;
  uint16_t size;
};

std::vector<uint8_t> getBinary(
  std::string filename,
  std::map<uint16_t, SymbolData>* symbols = nullptr);

#endif // MC3_ELF_HANDLER_ELF_HPP
