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
  -h, --help                     Show this help text
  -d [device], --debug [device]  Show debug window for specified device, or cpu if omitted
    Possible devices: all, cpu, ram, vga, tty, hdd, keyboard, mouse, speaker
  -p, --protect                  Protect memory regions and halt the processor if memory is illegaly written to

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
      i++;
      arg = argv[i];
      if (arg == "all")
      {
        debugWindow.enabledModes.ram = true;
        debugWindow.enabledModes.vga = true;
        debugWindow.enabledModes.tty = true;
        debugWindow.enabledModes.hdd = true;
        debugWindow.enabledModes.keyboard = true;
        debugWindow.enabledModes.mouse = true;
        debugWindow.enabledModes.speaker = true;
        debugWindow.enabledModes.cpu = true;
      } else if (arg == "ram")
      {
        debugWindow.enabledModes.ram = true;
      } else if (arg == "vga")
      {
        debugWindow.enabledModes.vga = true;
      } else if (arg == "tty")
      {
        debugWindow.enabledModes.tty = true;
      } else if (arg == "hdd")
      {
        debugWindow.enabledModes.hdd = true;
      } else if (arg == "keyboard")
      {
        debugWindow.enabledModes.keyboard = true;
      } else if (arg == "mouse")
      {
        debugWindow.enabledModes.mouse = true;
      } else if (arg == "speaker")
      {
        debugWindow.enabledModes.speaker = true;
      } else
      {
        debugWindow.enabledModes.cpu = true;
        i--;
      }
    } else if (arg == "--protect" || arg == "-p")
    {

    } else
    {
      filename = arg;
    }
  }
}
