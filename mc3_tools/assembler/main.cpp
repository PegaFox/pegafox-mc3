#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

std::string inputFilename = "../../mc3_programs/sandbox.s";
std::string outputFilename = "a.out";

bool rawBinary = false;

#include "handle_args.hpp"

#include "preprocessor.hpp"

#include "tokenize.hpp"

#include "../elf_handler/elf.hpp"
#include "../elf_handler/elf.cpp"

#include "assemble.hpp"

#include "format_elf.hpp"

int main(int argc, char *argv[])
{
  handleArgs(argc, argv);

  if (inputFilename.empty())
  {
    std::cout << "No input file specified.\n";
    return -1;
  }

  std::ifstream inputFile(inputFilename);

  std::stringstream assembly;
  assembly << inputFile.rdbuf();

  inputFile.close();

  std::string processed = preprocess(assembly.str());

  std::vector<std::string> tokens = tokenize(processed);

  /*for (auto token: tokens)
  {
    std::cout << "\"" << token << "\"\n";
  }*/

  std::vector<ELF32::SymbolData> symbolTable;
  std::vector<uint8_t> binary = assemble(tokens, &symbolTable);

  if (!rawBinary)
  {
    binary = formatELF(binary, symbolTable);
  }

  std::filebuf file;
  file.open(outputFilename, std::ios::out | std::ios::binary);

  file.sputn((char*)binary.data(), binary.size());

  file.close();

  return 0;
}
