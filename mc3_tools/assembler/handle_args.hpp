#include <string>
#include <iostream>

extern std::string inputFilename;
extern std::string outputFilename;

extern bool rawBinary;

void showHelp()
{
  std::cout << R"(
mc3as - assembler for the PegaFox MC3 CPU

Usage:
  mc3as [options] <file.s> [-o outputFile]

Creates a flat binary from a mc3 assembly file.
Input file must have a .s file extension.

Options:
  -h, --help                              Show this help text
  -r, --raw                               Generate a raw binary, if this is not included, the assembler generates an executable with an ELF-like segment descriptor format
  -o <outputFile>, --output <outputFile>  Specify output file, defaults to a.out

Examples:

  Show this help text:
    mc3as -h

  Print disassembly of input file:
    mc3as <file.s>
)";
}

void handleArgs(int argc, char* argv[])
{
  for (int i = 0; i < argc; i++)
  {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help")
    {
      showHelp();
    } else if (arg == "-r" || arg == "--raw")
    {
      rawBinary = true;
    } else if (arg.find(".s") == arg.size() - 2)
    {
      inputFilename = arg;
    } else if (arg ==  "-o" || arg == "--output")
    {
      i++;
      outputFilename = argv[i];
    }
  }
}
