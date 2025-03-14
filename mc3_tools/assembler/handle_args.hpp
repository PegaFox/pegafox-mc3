void showHelp()
{
  std::cout << R"(
mc3as - assembler for the PegaFox MC3 CPU

Usage:
  mc3as [options] <file.s> [-o outputFile]

Creates a flat binary from a mc3 assembly file.
Input file must have a .s file extension.

Options:
  -h, --help  Show this help text
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

    if (arg.find(".s") == arg.size() - 2)
    {
      inputFilename = arg;
    } else if (arg == "--output" || arg ==  "-o")
    {
      i++;
      outputFilename = argv[i];
    } else if (arg == "--help" || arg == "-h")
    {
      showHelp();
    }
  }
}