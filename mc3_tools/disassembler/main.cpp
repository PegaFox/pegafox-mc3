#include <iostream>
#include <cstdint>

#define USE_PEGAFOX_UTILS_IMPLEMENTATION
#include <pegafox/utils.hpp>

std::string filename = "a.out";

std::array<uint8_t, UINT16_MAX+1> program;

#include "handle_args.hpp"

#include "../mc3_utils.hpp"

#include "disassemble_instruction.hpp"

int main(int argc, char* argv[])
{
  handleArgs(argc, argv);

  if (!filename.empty())
  {
    std::string fileText = pf::getFileText(filename);

    uint16_t end = fileText.size();

    std::copy(fileText.begin(), fileText.end(), program.begin());

    for (uint16_t i = 0; i < end; i += 2)
    {
      std::cout << pf::intToHexStr(uint16_t(i), "") << '\t';

      std::cout << (program[i] >> 7) << ((program[i] >> 6) & 1) << ((program[i] >> 5) & 1) << ((program[i] >> 4) & 1) << ((program[i] >> 3) & 1);
      switch (opcodeTypes[program[i] >> 3])
      {
        case OperationType::NoOperands:
          std::cout << ' ' << pf::intToHexStr(uint16_t((uint16_t(program[i] & 0x7) << 8) | program[i+1])).erase(2, 1);
          break;
        case OperationType::OneOperand:
          std::cout << ' ' << (program[i] & 0x7);
          std::cout << ' ' << pf::intToHexStr(program[i+1]);
          break;
        case OperationType::ValueOperand:
          std::cout << ' ' << (program[i] & 0x7);
          std::cout << ' ' << pf::intToHexStr(uint8_t(program[i]));
          break;
        case OperationType::RegValue:
          std::cout << ' ' << (program[i] & 0x7);
          std::cout << ' ' << (program[i+1] >> 5);
          std::cout << ' ' << pf::intToHexStr(uint8_t(program[i+1] & 0x1F));
          break;
        case OperationType::RegValueExt:
          std::cout << ' ' << (program[i] & 0x7);
          std::cout << ' ' << (program[i+1] >> 6);
          std::cout << ' ' << pf::intToHexStr(uint8_t(program[i+1] & 0x3F));
          break;
      }
      std::cout << '\t' << disassembleInstruction(program[i], program[i+1]) << '\n';

      if (i == 255) break;
    }

  } else {
    std::clog << "Disassembly failed, please specify an input file\n";
  }

  return 0;
}
