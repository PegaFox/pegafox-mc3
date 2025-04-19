#include <iostream>
#include <string>

#define USE_PEGAFOX_UTILS_IMPLEMENTATION
#include <pegafox/utils.hpp>

std::string inputFilename = "../../mc3_programs/sandbox.s";
std::string outputFilename = "a.out";

bool rawBinary = false;

#include "handle_args.hpp"

#include "preprocessor.hpp"

#include "tokenize.hpp"

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

  std::string assembly = pf::getFileText(inputFilename);

  std::string processed = preprocess(assembly);

  std::vector<std::string> tokens = tokenize(processed);

  std::vector<uint8_t> binary = assemble(tokens);

  /*if (!rawBinary)
  {
    binary = formatELF(binary);
  }*/

  std::filebuf file;
  file.open(outputFilename, std::ios::out | std::ios::binary);

  file.sputn((char*)binary.data(), binary.size());

  file.close();

  return 0;
}
