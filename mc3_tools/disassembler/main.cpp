#include <iostream>
#include <vector>
#include <format>
#include <map>
#include <fstream>

#include "handle_args.hpp"

#include "../mc3_utils.hpp"

#include "../elf_handler/elf.hpp"
#include "../elf_handler/elf.cpp"

#include "disassemble_instruction.hpp"

int main(int argc, char* argv[])
{
  std::string filename = "a.out";
  
  handleArgs(argc, argv, filename);

  if (filename.empty())
  {
    std::clog << "Disassembly failed, please specify an input file\n";
    return 10;
  }

  //std::array<uint8_t, UINT16_MAX+1> program;
  std::map<uint16_t, SymbolData> symbols;
  const std::vector<uint8_t> program = getBinary(filename, &symbols);

  if (program.empty())
  {
    std::clog << "Disassembly failed, could not open file \"" << 
      filename << "\"\n";
    return 11;
  }

  std::map<uint16_t, SymbolData>::iterator currentVar = symbols.end();
  for (uint16_t i = 0; i < program.size(); i += 2)
  {
    if (currentVar != symbols.end())
    {
      if (i >= currentVar->first + currentVar->second.size)
      {
        if (symbols.contains(i))
        {
          std::cout << "} ";
        } else
        {
          std::cout << "}\n";
          currentVar = symbols.end();
        }
      }
    }

    if (symbols.contains(i))
    {
      std::cout << symbols[i].name;
      if (symbols[i].type == SymbolData::Variable)
      {
        std::cout << "\n{\n";
        currentVar = symbols.find(i);
      } else if (symbols[i].type == SymbolData::Label)
      {
        std::cout << ":\n";
      }
    }

    //if (currentVar != symbols.end())
    //{
    //  std::cout << "  ";
    //}

    std::cout << "| " << std::format("{:>4X}", uint16_t(i)) << " | ";

    std::cout << std::format("{:02X}", program[i] >> 3) << " ";

    uint8_t partialDisassemblyWidth = 8;
    switch (opcodeTypes[program[i] >> 3])
    {
      case OperationType::NoOperands:
        //std::cout << std::format("{:X}", uint16_t((uint16_t(program[i] & 0x7) << 8) | program[i+1])).erase(2, 1);
        std::cout << std::format("{1:<{0}X}", partialDisassemblyWidth, uint16_t((uint16_t(program[i] & 0x7) << 8) | program[i+1]));
        break;
      case OperationType::OneOperand:
        std::cout << std::format("{1:<{0}s}", partialDisassemblyWidth, std::format("{:X} {:X}", program[i] & 0x7, program[i+1]));
        break;
      case OperationType::ValueOperand:
        std::cout << std::format("{1:<{0}s}", partialDisassemblyWidth, std::format("{:X} {:X}", program[i] & 0x7, program[i+1]));
        break;
      case OperationType::Reg3: {
        std::string partialDisassemblyStr = std::format("{:X} {:X}", program[i] & 0x7, program[i+1] >> 5);
        if (program[i+1] & 1)
        {
          partialDisassemblyStr += std::format(" {:X} {:X}", uint8_t(program[i+1] & 0x1E) >> 1, program[i+1] & 1);
        } else
        {
          partialDisassemblyStr += std::format(" {:X} {:X}", (program[i+1] >> 2) & 0x07, program[i+1] & 3);
        }

        std::cout << std::format("{1:<{0}s}", partialDisassemblyWidth, partialDisassemblyStr);
        break;
      } case OperationType::RegValue:
        std::cout << std::format("{1:<{0}s}", partialDisassemblyWidth, std::format("{:X} {:X} {:X}", program[i] & 0x7, program[i+1] >> 6, uint8_t(program[i+1] & 0x3F)));
        break;
    }
    std::cout << " | " << std::format("{:<20s}", disassembleInstruction(program[i], program[i+1])) << " |\n";

    //if (i == 255) break;
  }

  if (currentVar != symbols.end())
  {
    std::cout << "}\n";
  }

  return 0;
}

