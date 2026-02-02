#include "elf.hpp"

#include <fstream>

const uint8_t ELF32::ident[EI_NIDENT] = {
  ELFMAG0,
  ELFMAG1,
  ELFMAG2,
  ELFMAG3,
  ELFCLASS32,
  ELFDATA2LSB,
  EV_CURRENT,
  ELFOSABI_NONE,
};

// Segments and sections can overlap

// This initializes a basic elf file
ELF32::ELF32(uint16_t entryPoint)
{
  this->file.clear();

  Elf32_Ehdr header = this->initHeader(entryPoint);

  this->insert(0, header);

  this->addSection(SHT_NULL, 0, 0);

  this->addSection(SHT_STRTAB, 0, 1); // We have to initialize the string table first before adding its name
  this->sectionHeader(1)->sh_name = this->addString(".dynstr");
}

// This loads an existing elf file
ELF32::ELF32(const uint8_t* begin, const uint8_t* end)
{
  this->file.clear();
  this->file.insert(this->file.begin(), begin, end);
}

// Type can be any PT_* value as defined in elf.h
Elf32_Addr ELF32::addSegment(Elf32_Word type, Elf32_Addr ramPos, Elf32_Addr size)
{
  // Preallocate space to avoid pointer invalidation
  this->file.reserve(this->file.size()+sizeof(Elf32_Phdr)+size);

  Elf32_Phdr sHeader{
    .p_type = type,			/* Segment type */
    .p_offset = (Elf32_Off)(this->file.size()+sizeof(Elf32_Phdr)),		/* Segment file offset */
    .p_vaddr = ramPos,		/* Segment virtual address */
    .p_paddr = ramPos,		/* Segment physical address */
    .p_filesz = size,		/* Segment size in file */
    .p_memsz = size,		/* Segment size in memory */
    .p_flags = PF_X | PF_W | PF_R,		/* Segment flags */
    .p_align = 1,		/* Segment alignment */
  };

  Elf32_Ehdr* header = this->header();

  if (header->e_phnum == 0)
  {
    header->e_phoff = header->e_ehsize;
  }

  // Insert header
  this->insert(
    header->e_phoff + header->e_phentsize*header->e_phnum,
    sHeader
  );

  // Append segment
  uint8_t segment[size];
  std::fill(segment, segment+size, 0);

  this->insert(this->file.size(), segment, segment + size);

  header->e_phnum++;
  return sHeader.p_offset;
}

// Type can be any SHT_* value as defined in elf.h
Elf32_Addr ELF32::addSection(
  Elf32_Word type,
  Elf32_Addr ramPos,
  Elf32_Addr size,
  const std::string& name)
{
  Elf32_Off realNameSize = name.empty() ? 0 : name.size()+1;  

  // Preallocate space to avoid pointer invalidation
  this->file.reserve(
    this->file.size() + 
    sizeof(Elf32_Shdr) + 
    size + 
    realNameSize
  );

  Elf32_Shdr sHeader{
    .sh_name = this->addString(name),		/* Section name (string tbl index) */
    .sh_type = type,		/* Section type */
    .sh_flags = SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR,		/* Section flags */
    .sh_addr = ramPos,		/* Section virtual addr at execution */
    .sh_offset = (Elf32_Off)(
      this->file.size() + 
      sizeof(Elf32_Shdr) // Don't need to add name size here because ields are initialized in order
    ),		/* Section file offset */
    .sh_size = size,		/* Section size in bytes */
    .sh_link = 0,		/* Link to another section */
    .sh_info = 0,		/* Additional section information */
    .sh_addralign = 1,		/* Section alignment */
    .sh_entsize = 0,		/* Entry size if section holds table */
  };

  Elf32_Ehdr* header = this->header();

  if (header->e_shnum == 0)
  {
    header->e_shoff = 
      header->e_ehsize + header->e_phentsize*header->e_phnum;
  }

  // Insert header
  this->insert(
    header->e_shoff + header->e_shentsize*header->e_shnum,
    sHeader
  );

  // Append segment
  uint8_t segment[size];
  std::fill(segment, segment+size, 0);

  this->insert(this->file.size(), segment, segment + size);

  header->e_shnum++;
  return sHeader.sh_offset;
}

void ELF32::addSymbolTable(SymbolData* symbols, Elf32_Word count)
{
  this->addSection(
    SHT_SYMTAB,
    0,
    (count+1)*sizeof(Elf32_Sym),
    ".symtab"
  );

  const Elf32_Half sectionCount = this->header()->e_shnum;
  Elf32_Shdr* sHeader = this->sectionHeader(sectionCount-1);

  sHeader->sh_entsize = sizeof(Elf32_Sym);
  sHeader->sh_link = 1;
  sHeader->sh_info = count+1;

  *(Elf32_Sym*)(this->data()+sHeader->sh_offset) = Elf32_Sym{0};

  for (Elf32_Word s = 0; s < count; s++)
  {
    Elf32_Sym symbol{
      .st_name = this->addString(symbols[s].name),		/* Symbol name (string tbl index) */
      .st_value = symbols[s].value,		/* Symbol value */
      .st_size = symbols[s].size,		/* Symbol size */
      .st_info = ELF32_ST_INFO(STB_GLOBAL, symbols[s].type),		/* Symbol type and binding */
      .st_other = STV_DEFAULT,		/* Symbol visibility */
      .st_shndx = 0,		/* Section index */
    };

    // We need to recalculate these pointers every time due to arraylist reallocation
    sHeader = this->sectionHeader(sectionCount-1);
    uint8_t* tableStart = this->data()+sHeader->sh_offset;

    *(Elf32_Sym*)(tableStart+sizeof(Elf32_Sym)*(s+1)) = symbol;
  }
}

ELF32::SymbolTable ELF32::getSymbolTable(Elf32_Half index)
{
  Elf32_Shdr* symHeader = this->sectionHeader(index);
  if (symHeader == nullptr || symHeader->sh_type != SHT_SYMTAB)
  {
    return {0, 0, 0};
  }

  Elf32_Shdr* strHeader = this->sectionHeader(symHeader->sh_link);

  return {
    .strings = strHeader->sh_offset,
    .symbols = symHeader->sh_offset,
    .symbolCount = (Elf32_Half)(symHeader->sh_size / symHeader->sh_entsize),
  };
}

bool ELF32::valid()
{
  // Check for \x7fELF magic number
  return std::equal(this->data(), this->data()+4, ident);
}

Elf32_Ehdr* ELF32::header()
{
  if (!this->valid())
  {
    return nullptr;
  }

  return (Elf32_Ehdr*)this->data();
}

Elf32_Phdr* ELF32::programHeader(Elf32_Half index)
{
  Elf32_Ehdr* header = this->header();

  if (index >= header->e_phnum)
  {
    return nullptr;
  }

  return (Elf32_Phdr*)(
    this->data() + header->e_phoff + header->e_phentsize*index
  );
}

// Section 0 is always null
// Section 1 is a string table
// The other sections can be user defined
Elf32_Shdr* ELF32::sectionHeader(Elf32_Half index)
{
  Elf32_Ehdr* header = this->header();

  if (index >= header->e_shnum)
  {
    return nullptr;
  }

  return (Elf32_Shdr*)(
    this->data() + header->e_shoff + header->e_shentsize*index
  );
}

uint8_t* ELF32::data()
{
  return this->file.data();
}

// Returns the size in bytes of this->data()
uint32_t ELF32::size()
{
  return this->file.size();
}

template <typename IIt>
Elf32_Addr ELF32::insert(Elf32_Addr pos, IIt begin, IIt end)
{
  Elf32_Addr fileSize = this->file.size();

  Elf32_Off off = 0;
  for (IIt item = begin; item != end; item++)
  {
    this->file.insert(
      this->file.begin()+pos+off,
      (uint8_t*)&(*item),
      (uint8_t*)(&(*item))+sizeof(*item)
    );
    
    off++;
  }

  syncPointers(pos, this->file.size() - fileSize, Filedata);
  
  return pos;
}

template <typename T>
Elf32_Addr ELF32::insert(Elf32_Addr pos, T data)
{
  this->file.insert(
    this->file.begin()+pos,
    (uint8_t*)&data,
    (uint8_t*)(&data)+sizeof(data)
  );

  AllocationType type = Header;
  switch (sizeof(data))
  {
    case sizeof(Elf32_Ehdr): type = Header; break;
    case sizeof(Elf32_Phdr): type = SegmentHeader; break;
    case sizeof(Elf32_Shdr): type = SectionHeader; break;
    default:                 type = Filedata; break;
  }

  syncPointers(pos, sizeof(data), type);
  
  return pos;
}

Elf32_Ehdr ELF32::initHeader(uint16_t entryPoint)
{
  Elf32_Ehdr header{
    .e_ident = {},	/* Magic number and other info */
    .e_type = ET_EXEC,			/* Object file type */
    .e_machine = EM_NONE,		/* Architecture */
    .e_version = EV_CURRENT,		/* Object file version */
    .e_entry = entryPoint,		/* Entry point virtual address */
    .e_phoff = 0,		/* Program header table file offset */
    .e_shoff = 0,		/* Section header table file offset */
    .e_flags = 0,		/* Processor-specific flags */
    .e_ehsize = sizeof(Elf32_Ehdr),		/* ELF header size in bytes */
    .e_phentsize = sizeof(Elf32_Phdr),		/* Program header table entry size */
    .e_phnum = 0,		/* Program header table entry count */
    .e_shentsize = sizeof(Elf32_Shdr),		/* Section header table entry size */
    .e_shnum = 0,		/* Section header table entry count */
    .e_shstrndx = 1,		/* Section header string table index */
  };

  std::copy(ident, ident + sizeof(ident), header.e_ident);

  return header;
}

Elf32_Addr ELF32::addString(std::string string)
{
  if (string.empty())
  {
    return 0;
  }

  Elf32_Shdr* stringHeader = this->sectionHeader(1);

  string.insert(string.begin(), '\0');

  Elf32_Addr pos = this->insert(
    stringHeader->sh_offset + stringHeader->sh_size-1,
    string.begin(),
    string.end()
  );

  // We increment it because the first character of the string is technically 0
  return pos - stringHeader->sh_offset + 1;
}

// This should be run after inserting or removing something from the center of the file
void ELF32::syncPointers(Elf32_Addr pos, Elf32_Off len, AllocationType type)
{
  Elf32_Ehdr* header = this->header();

  syncPointer(pos, len, header->e_phoff, true);
  syncPointer(pos, len, header->e_shoff, type != SegmentHeader);

  bool appendFiledata = type == Filedata;
  for (Elf32_Half ph = 0; ph < header->e_phnum; ph++)
  {
    Elf32_Phdr* pHeader = this->programHeader(ph);

    if (
      !syncPointer(pos, len, pHeader->p_offset, appendFiledata) &&
      pos < pHeader->p_offset+pHeader->p_filesz)
    {
      pHeader->p_filesz += len;
      pHeader->p_memsz += len;
    }
  }

  for (Elf32_Half sh = 0; sh < header->e_shnum; sh++)
  {
    Elf32_Shdr* sHeader = this->sectionHeader(sh);

    if (
      !syncPointer(pos, len, sHeader->sh_offset, appendFiledata) &&
      pos < sHeader->sh_offset+sHeader->sh_size)
    {
      sHeader->sh_size += len;
    }
  }
}

// Append specifies what to do if pos == ptr
// If append is true, ptr == pos will leave ptr alone
// Returns whether pointer was changed
bool ELF32::syncPointer(
  Elf32_Addr pos,
  Elf32_Off len,
  Elf32_Addr& ptr,
  bool append)
{
  if (ptr-append >= pos)
  {
    ptr += len;

    return true;
  }

  return false;
}

std::vector<uint8_t> getBinary(
  std::string filename,
  std::map<uint16_t, SymbolData>* symbols)
{
  std::filebuf inputFile;
  inputFile.open(filename, std::ios::in | std::ios::binary);
  if (inputFile.is_open())
  {
    uint16_t size = inputFile.pubseekoff(0, std::ios::end);
    inputFile.pubseekoff(0, std::ios::beg);

    std::vector<uint8_t> fileData(size);
    inputFile.sgetn((char*)fileData.data(), size);

    inputFile.close();

    ELF32 elfFile(fileData.data(), fileData.data()+size);
    if (elfFile.valid())
    {
      // This is safe to do because the ELF32 constructor copies the file contents
      fileData.clear();

      for (uint16_t p = 0; Elf32_Phdr* pHeader = elfFile.programHeader(p); p++)
      {
        if (pHeader->p_type != PT_LOAD)
        {
          continue;
        }
        
        if (fileData.size() < pHeader->p_vaddr + pHeader->p_memsz)
        {
          fileData.resize(pHeader->p_vaddr + pHeader->p_memsz);
        }

        uint8_t* startPtr = elfFile.data()+pHeader->p_offset;
        std::copy(
          startPtr,
          startPtr + pHeader->p_filesz,
          fileData.data() + pHeader->p_vaddr
        );
      }

      if (symbols == nullptr)
      {
        return fileData;
      }

      for (uint16_t s = 0; Elf32_Shdr* sHeader = elfFile.sectionHeader(s); s++)
      {
        if (sHeader->sh_type != SHT_SYMTAB)
        {
          continue;
        }
        ELF32::SymbolTable symTab = elfFile.getSymbolTable(s);
        
        const char* strTable = (const char*)elfFile.data() + symTab.strings;

        Elf32_Sym* symbolList = (Elf32_Sym*)(elfFile.data() + symTab.symbols);
        for (uint16_t sym = 0; sym < symTab.symbolCount; sym++)
        {
          Elf32_Sym symbol = symbolList[sym];
          if (*(strTable+symbol.st_name) == 0)
          {
            continue;
          }
          (*symbols)[symbol.st_value] = {
            .type = ELF32_ST_TYPE(symbol.st_info) == STT_OBJECT ?
              SymbolData::Variable :
              SymbolData::Label,
            .name = strTable + symbol.st_name,
            .size = (uint16_t)symbol.st_size,
          };
        }
      }
    }

    return fileData;
  }

  return {};
}

