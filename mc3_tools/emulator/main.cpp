#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cstring>
#include <iostream>
#include <string>

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

VirtMachine vm;
RAM<0xFF00> ram;
HDD hdd("drive.img", 2880);
TTY tty;
VGA vga;
Keyboard keyboard;
Mouse mouse;
Speaker speaker;

std::string filename;

#include <sys/ioctl.h>

#include "handle_args.hpp"

#include "../elf_handler/elf.hpp"
#include "../elf_handler/elf.cpp"

#include "clock.hpp"

#include "debug_window.hpp"

DebugWindow debugWindow;
/*
Memory layout

With no particular size: Boot code, Data
0xFE00-0xFEFF: RAM
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

  Clock vSyncClock;

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

  std::vector<uint8_t> binary = getBinary(filename, &debugWindow.symbols);
  std::copy(binary.begin(), binary.end(), ram.memory);

  bool running = true;
  while (running)
  {
    bool newFrame = false;
    vSyncClock.get_fps(false);
    if (vSyncClock.deltaTime > 0.015f)
    {
      newFrame = true;
      vSyncClock.get_fps();
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
    {
      break;
    }

    if (newFrame)
    {
      vga.update();
    }

    // sends interrupt 0x60 when a key event occurs.
    if (keyboard.update())
    {
      vm.hardwareInterrupt(0x60);
    }

    mouse.update();

    if (!debugWindow.CPUpaused())
    {
      running = vm.tickClock();
    }

    if (newFrame)
    {
      debugWindow.update();
    }

    /*if (std::memcmp(ram.memory, program.data(), 0xF1) != 0)
    {
      std::cerr << "invalid memory write\n";
      debugWindow.pause = true;
    }*/
    //std::cout << '\r' << ((uint16_t)ram.memory[0x0011] | ((uint16_t)ram.memory[0x0012] << 8)) << std::flush;
  }

  return 0;
}
