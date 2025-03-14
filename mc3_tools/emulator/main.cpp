#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <deque>

#define USE_PEGAFOX_UTILS_IMPLEMENTATION
#include <pegafox/utils.hpp>

#include "emu-utils/bus.hpp"
#include "emu-utils/ram.hpp"
#include "emu-utils/rom.hpp"

#include "emu-utils/hdd.hpp"
#include "emu-utils/tty.hpp"
#include "emu-utils/vga.hpp"
#include "emu-utils/keyboard.hpp"
#include "emu-utils/mouse.hpp"
#include "emu-utils/speaker.hpp"

#include "virt_machine.hpp"

#include "debug_window.hpp"

DebugWindow debugWindow;

std::string filename;

#include <sys/ioctl.h>

#include "handle_args.hpp"

VirtMachine vm;

/*
Memory layout

With no particular size: Boot code, Data
0xFE00-0xFEFF: Stack
0xFF00-0xFFFF: I/O
  hdd - 7 bytes
  TTY - 1 byte
  VGA - 6 bytes
  Keyboard - 2 bytes
  Mouse - 4 bytes
  Speaker - 3 bytes
*/

int main(int argc, char *argv[])
{
  handleArgs(argc, argv);

  RAM<0xFF00> ram;

  HDD hdd("drive.img");
  TTY tty;
  VGA vga;
  Keyboard keyboard;
  Mouse mouse;
  Speaker speaker;

  vm.bus.connect(&ram, 0x0000, 0xFEFF);
  vm.bus.connect(&hdd, 0xFF00, 0xFF06);
  vm.bus.connect(&tty, 0xFF07, 0xFF07);
  vm.bus.connect(&vga, 0xFF08, 0xFF0D);
  vm.bus.connect(&keyboard, 0xFF0E, 0xFF0F);
  vm.bus.connect(&mouse, 0xFF10, 0xFF13);
  vm.bus.connect(&speaker, 0xFF14, 0xFF16);

  if (filename.empty())
  {
    std::cout << "No input file specified.\n";
    return 1;
  }

  std::string program = pf::getFileText(filename);

  std::copy(program.begin(), program.end(), ram.memory);

  bool running = true;
  while (running)
  {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
    {
      break;
    }

    vga.update();

    // sends interrupt 0x60 when a key event occurs.
    if (keyboard.update())
    {
      vm.hardwareInterrupt(0x60);
    }

    mouse.update();

    if (vm.pc >= 0xFFFE)
    {
      running = false;
    }

    vm.tickClock();

    if (vm.pc != 0x0000)
    {
      running = true;
    }

    debugWindow.update();

    //std::cout << '\r' << ((uint16_t)ram.memory[0x0011] | ((uint16_t)ram.memory[0x0012] << 8)) << std::flush;
  }

  return 0;
}
