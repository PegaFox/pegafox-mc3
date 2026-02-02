#include <iostream>
#include <string>
#include "debug_window.hpp"

extern DebugWindow debugWindow;
extern std::string filename;

void showHelp()
{
  std::cout << R"(
mc3emu - emulator for the PegaFox MC3 CPU

Usage:
  mc3emu [options] <file>

Options:
  -h, --help     Show this help text
  -d, --debug    Show debug window
  -p, --protect  Protect memory regions and halt the processor if memory is illegaly written to

Examples:

  Show this help text:
    mc3emu -h

  Start emulation with default configuration and input file loaded at 0x0000:
    mc3emu <file>
)";
}

void handleArgs(int argc, char* argv[])
{
  if (argc == 1)
  {
    showHelp();
    exit(0);
  }

  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h")
    {
      showHelp();
    } else if (arg == "--debug" || arg == "-d")
    {
      debugWindow.create();
    } else if (arg == "--protect" || arg == "-p")
    {

    } else
    {
      filename = arg;
    }
  }
}
