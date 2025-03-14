void showHelp()
{
  std::cout << R"(
mc3dump - disassembler for the PegaFox MC3 CPU

Usage:
  mc3dump [options] <file>

Options:
  -h, --help  Show this help text

Examples:

  Show this help text:
    mc3dump -h

  Print disassembly of input file:
    mc3dump <file>
)";
}

void handleArgs(int argc, char* argv[])
{
  if (argc == 1)
  {
    showHelp();
    exit(0);
  }

  for (uint32_t i = 1; i < argc; i++)
  {
    std::string arg(argv[i]);

    if (arg == "--help" || arg == "-h")
    {
      showHelp();
    } else
    {
      filename = arg;
    }
  }
}