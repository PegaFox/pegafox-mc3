#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <list>
#include <memory>
#include <algorithm>
#include <vector>
#include <map>
#include <array>
#include <set>

/*

  this compiler is currently little endian

  gcc -S -O -fno-asynchronous-unwind-tables -fcf-protection=none main.c

*/

#include "c-compiler-lib/C_compiler.hpp"

#include "assemble_ir.hpp"

#include "optimize_asm.hpp"

#include "../assembler/assemble.hpp"

#include "../disassembler/disassemble_instruction.hpp"

int main(int argc, char* argv[])
{
  Compiler compiler;

  /*#ifndef NDEBUG
  {
    argc = 4;

    char* args[4] = {
      "./C_compiler",
      "../tests/counting.c",
      "-O",
      "-I../include"
    };
    argv = args;
  }
  #endif // NDEBUG*/
  
  compiler.typeSizes.pointerSize = 2;

  compiler.inputFilenames = {
    "../C_compiler/libc/lib/memset.c",
    "../C_compiler/libc/lib/memcpy.c",
    "../C_compiler/libc/lib/srand.c",
    "../C_compiler/libc/lib/rand.c"
  };

  compiler.includeDirs.emplace_back("../C_compiler/libc/include/");

  IRprogram irCode = compiler.compileFromArgs(argc, argv);

  //std::cout << "Intermediate Representation:\n" << compiler.printIR(irCode) << '\n';

  std::vector<std::string> assembly = assembleIR(irCode);

  std::cout << "Assembly: \n";
  uint16_t irFunctionIndex = 0;
  uint16_t irOperationIndex = 0;
  for (std::vector<std::string>::const_iterator token = assembly.begin(); token != assembly.end(); token++)
  {
    if (!token->empty())
    {
      if (
        *token == "var" ||
        *token == "or" || *token == "and" || *token == "xor" || *token == "lsh" ||
        *token == "rsh" || *token == "add" || *token == "sub" || *token == "or" ||
        *token == "and" || *token == "xor" || *token == "lsh" || *token == "rsh" ||
        *token == "add" || *token == "sub" || *token == "set" || *token == "set" ||
        *token == "set" || *token == "put" || *token == "put" || *token == "jz" ||
        *token == "jnz" || *token == "jc" || *token == "jnc" || *token == "js" ||
        *token == "jns" || *token == "jo" || *token == "jno" || token->back() == ':')
      {
        std::cout << "\n";
      } else if (*token == "@" || *token == "+" || *token == "-")
      {
        std::cout << "\b";
      }

      std::cout << *token;

      if (*token != "@" && *token != "+" && *token != "-")
      {
        std::cout << " ";
      }
    } else
    {
      assembly.erase(token--);
      std::cout << "\n\n" << compiler.printIRoperation(irCode.program[irFunctionIndex].body[irOperationIndex]);

      irOperationIndex++;
      if (irOperationIndex >= irCode.program[irFunctionIndex].body.size())
      {
        irOperationIndex = 0;
        irFunctionIndex++;
      }
    }
  }

  //std::cout << "Intermediate Representation:\n" << compiler.printIR(irCode) << '\n';

  std::vector<uint8_t> binary = assemble(assembly);

  std::filebuf file;
  file.open(compiler.outputFilename, std::ios::out | std::ios::binary);

  file.sputn((char*)binary.data(), binary.size());

  file.close();

  return 0;
}

