#ifndef EMULATOR_DEBUG_WINDOW_HPP
#define EMULATOR_DEBUG_WINDOW_HPP

#include <iostream>
#include <format>
#include <SFML/Graphics/RenderWindow.hpp>
#include "gui-lib/gui.hpp"
#include "../disassembler/disassemble_instruction.hpp"
#include "virt_machine.hpp"
#include "emu-utils/ram.hpp"

extern VirtMachine vm;
extern RAM<0xFF00> ram;

class DebugWindow
{
  public:
    struct Modes
    {
      bool cpu: 1;
      bool ram: 1;
      bool vga: 1;
      bool tty: 1;
      bool hdd: 1;
      bool keyboard: 1;
      bool mouse: 1;
      bool speaker: 1;
    } enabledModes = {0, 0, 0, 0, 0, 0, 0, 0};

    bool pause = false;

    DebugWindow()
    {
      
    }

    bool CPUpaused()
    {
      return pause;
    }

    void create()
    {
      window.create(sf::VideoMode(sf::Vector2u(1024, 512)), "Debug Window");
      pause = true;

      if (!pfui::Paragraph::font.openFromFile(/*"3270NerdFontMono-Regular.ttf"*/"PublicPixel.ttf"))
      {
        std::clog << "Warning: debug window failed to load default font, text rendering mey not work correctly\n";
      }

      std::string_view winNames[8] = {
        "CPU",
        "RAM",
        "VGA",
        "TTY",
        "HDD",
        "KEYBOARD",
        "MOUSE",
        "SPEAKER"
      };

      for (uint8_t w = 0; w < subWinCount; w++)
      {
        subWindows[w].title = winNames[w];
        subWindows[w].size = glm::vec2(1.0f, 1.93f);
        subWindows[w].pos = glm::vec2(-0.5f + w, 0.035f);

        pfui::Paragraph* winData = new pfui::Paragraph;
        subWindows[w].addChild(winData);
        winData->wrapMode = pfui::Paragraph::WrapMode::None;
        winData->size = glm::vec2(0.09f);
        winData->pos = glm::vec2(-0.98f);
      }
    }

    void update()
    {
      if (window.isOpen())
      {
        while (std::optional<sf::Event> event = window.pollEvent())
        {
          pfui::GUIElement::getEvent(event);
          if (const sf::Event::KeyReleased* key = event->getIf<sf::Event::KeyReleased>())
          {
            if (key->code == sf::Keyboard::Key::P)
            {
              pause = !pause;
            } else if (pause && key->code == sf::Keyboard::Key::S)
            {
              vm.tickClock();
            } else if (key->code == sf::Keyboard::Key::I)
            {
              vm.hardwareInterrupt(0xFF);
            }
          } else if (const sf::Event::MouseWheelScrolled* scroll = event->getIf<sf::Event::MouseWheelScrolled>())
          {
            float delta = scroll->delta;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            {
              delta *= 16;
            }
            if (activeWindow && activeWindow->getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))))
            {
              uint32_t* drawStart = &((pfui::Paragraph*)(*activeWindow)[0])->drawStart;
              *drawStart = glm::max(float(*drawStart)-delta, 0.0f);
            } else
            {
              for (uint8_t w = 0; w < subWinCount; w++)
              {
                if (subWindows[w].getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))))
                {
                  uint32_t* drawStart = &((pfui::Paragraph*)subWindows[w][0])->drawStart;
                  *drawStart = glm::max(float(*drawStart)-delta, 0.0f);
                  activeWindow = subWindows+w;
                  break;
                }
              }
            }
          } else if (const sf::Event::MouseButtonPressed* button = event->getIf<sf::Event::MouseButtonPressed>())
          {
            if (!activeWindow || !activeWindow->getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))))
            {
              for (uint8_t w = 0; w < subWinCount; w++)
              {
                if (subWindows[w].getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))))
                {
                  activeWindow = subWindows+w;
                  break;
                }
              }
            }
          }
        }

        window.clear(sf::Color::Black);

        if (enabledModes.cpu)
        {
          std::string winData;
          winData += "m0:" + std::format("{:X}", vm.regs[0]) + " ";
          winData += "m1:" + std::format("{:X}", vm.regs[1]) + "\n";
          winData += "m2:" + std::format("{:X}", vm.regs[2]) + " ";
          winData += "m3:" + std::format("{:X}", vm.regs[3]) + "\n";

          winData += "d0:" + std::format("{:X}", vm.regs[4]) + " ";
          winData += "d1:" + std::format("{:X}", vm.regs[5]) + "\n";
          winData += "d2:" + std::format("{:X}", vm.regs[6]) + " ";
          winData += "d3:" + std::format("{:X}", vm.regs[7]) + "\n";

          winData += "pc:" + std::format("{:X}", vm.pc) + " ";
          winData += "iv:" + std::format("{:X}", vm.intVec) + "\n";

          winData += "c:" + std::to_string(vm.flags.carry) + " ";
          winData += "o:" + std::to_string(vm.flags.overflow) + " ";
          winData += "z:" + std::to_string(vm.flags.zero) + " ";
          winData += "s:" + std::to_string(vm.flags.sign) + " ";

          uint8_t bitsSet = vm.flags.bitsSet;
          winData += "b:" + std::string(1, std::format("{:X}", bitsSet).back()) + "\n";
        
          winData += "IntBackup:\n";
          winData += "m0:" + std::format("{:X}", vm.intState.regs[0]) + " ";
          winData += "m1:" + std::format("{:X}", vm.intState.regs[1]) + "\n";
          winData += "m2:" + std::format("{:X}", vm.intState.regs[2]) + " ";
          winData += "m3:" + std::format("{:X}", vm.intState.regs[3]) + "\n";

          winData += "d0:" + std::format("{:X}", vm.intState.regs[4]) + " ";
          winData += "d1:" + std::format("{:X}", vm.intState.regs[5]) + "\n";
          winData += "d2:" + std::format("{:X}", vm.intState.regs[6]) + " ";
          winData += "d3:" + std::format("{:X}", vm.intState.regs[7]) + "\n";

          winData += "pc:" + std::format("{:X}", vm.intState.pc) + " ";
          winData += "i:" + std::to_string(vm.inInterrupt) + "\n";

          winData += "c:" + std::to_string(vm.intState.flags.carry) + " ";
          winData += "o:" + std::to_string(vm.intState.flags.overflow) + " ";
          winData += "z:" + std::to_string(vm.intState.flags.zero) + " ";
          winData += "s:" + std::to_string(vm.intState.flags.sign) + " ";

          bitsSet = vm.intState.flags.bitsSet;
          winData += "b:" + std::string(1, std::format("{:X}", bitsSet).back()) + "\n";

          for (uint8_t i = 0; i < vm.intQueue.size(); i++)
          {
            winData += "q" + std::to_string(i) + ":" + std::format("{:X}", vm.intQueue[i]) + (i%2 ? "\n" : " ");
          }
          
          ((pfui::Paragraph*)subWindows[0][0])->text.setString(winData);
        }

        if (enabledModes.ram)
        {
          std::string winData;

          for (uint32_t b = ((pfui::Paragraph*)subWindows[1][0])->drawStart; b < ((pfui::Paragraph*)subWindows[1][0])->drawStart+22; b++)
          {
            winData += std::format("{:04X}:{:02X}", uint16_t(b), ram.memory[b]);
            if (b%2)
            {
              winData += (b == vm.pc+1 ? ">" : " ") + disassembleInstruction(ram.memory[b-1], ram.memory[b]);
            }
            winData += "\n";
          }

          ((pfui::Paragraph*)subWindows[1][0])->text.setString(winData);
        }

        if (enabledModes.vga)
        {

        }

        if (enabledModes.tty)
        {

        }

        if (enabledModes.hdd)
        {

        }

        if (enabledModes.keyboard)
        {
        
        }

        if (enabledModes.mouse)
        {

        }

        if (enabledModes.speaker)
        {

        }

        for (uint8_t w = 0; w < subWinCount; w++)
        {
          if (activeWindow != subWindows+w)
          {
            subWindows[w].draw(window);
          }
        }

        if (activeWindow)
        {
          activeWindow->draw(window);
        }

        window.display();
      }
    }
  private:
    bool step = false;
    pfui::Window* activeWindow = nullptr;

    static const uint8_t subWinCount = 2;
    pfui::Window subWindows[subWinCount];  

    sf::RenderWindow window;
};

#endif // EMULATOR_DEBUG_WINDOW_HPP
