#ifndef EMULATOR_DEBUG_WINDOW_HPP
#define EMULATOR_DEBUG_WINDOW_HPP

#include <iostream>
#include <SFML/Graphics/RenderWindow.hpp>
#include "gui-lib/gui.hpp"
#include "../disassembler/disassemble_instruction.hpp"

extern VirtMachine vm;

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
          winData += "m0:" + pf::intToHexStr(vm.regs[0]) + " ";
          winData += "m1:" + pf::intToHexStr(vm.regs[1]) + "\n";
          winData += "m2:" + pf::intToHexStr(vm.regs[2]) + " ";
          winData += "m3:" + pf::intToHexStr(vm.regs[3]) + "\n";

          winData += "d0:" + pf::intToHexStr(vm.regs[4]) + " ";
          winData += "d1:" + pf::intToHexStr(vm.regs[5]) + "\n";
          winData += "d2:" + pf::intToHexStr(vm.regs[6]) + " ";
          winData += "d3:" + pf::intToHexStr(vm.regs[7]) + "\n";

          winData += "pc:" + pf::intToHexStr(vm.pc) + " ";
          winData += "iv:" + pf::intToHexStr(vm.intVec) + "\n";

          winData += "c:" + std::to_string(vm.flags.carry) + " ";
          winData += "o:" + std::to_string(vm.flags.overflow) + " ";
          winData += "z:" + std::to_string(vm.flags.zero) + " ";
          winData += "s:" + std::to_string(vm.flags.sign) + " ";
          winData += "b:0x" + std::string(1, pf::intToHexStr(vm.flags.bitsSet, "").back()) + "\n";
        
          winData += "IntBackup:\n";
          winData += "m0:" + pf::intToHexStr(vm.intState.regs[0]) + " ";
          winData += "m1:" + pf::intToHexStr(vm.intState.regs[1]) + "\n";
          winData += "m2:" + pf::intToHexStr(vm.intState.regs[2]) + " ";
          winData += "m3:" + pf::intToHexStr(vm.intState.regs[3]) + "\n";

          winData += "d0:" + pf::intToHexStr(vm.intState.regs[4]) + " ";
          winData += "d1:" + pf::intToHexStr(vm.intState.regs[5]) + "\n";
          winData += "d2:" + pf::intToHexStr(vm.intState.regs[6]) + " ";
          winData += "d3:" + pf::intToHexStr(vm.intState.regs[7]) + "\n";

          winData += "pc:" + pf::intToHexStr(vm.intState.pc) + " ";
          winData += "i:" + std::to_string(vm.inInterrupt) + "\n";

          winData += "c:" + std::to_string(vm.intState.flags.carry) + " ";
          winData += "o:" + std::to_string(vm.intState.flags.overflow) + " ";
          winData += "z:" + std::to_string(vm.intState.flags.zero) + " ";
          winData += "s:" + std::to_string(vm.intState.flags.sign) + " ";
          winData += "b:0x" + std::string(1, pf::intToHexStr(vm.intState.flags.bitsSet, "").back()) + "\n";

          for (uint8_t i = 0; i < vm.intQueue.size(); i++)
          {
            winData += "q" + std::to_string(i) + ":" + pf::intToHexStr(vm.intQueue[i]) + (i%2 ? "\n" : " ");
          }
          
          ((pfui::Paragraph*)subWindows[0][0])->text.setString(winData);
        }

        if (enabledModes.ram)
        {
          std::string winData;

          for (uint32_t b = ((pfui::Paragraph*)subWindows[1][0])->drawStart; b < ((pfui::Paragraph*)subWindows[1][0])->drawStart+22; b++)
          {
            winData += pf::intToHexStr(uint16_t(b), "") + ":" + pf::intToHexStr(ram.memory[b], "");
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
    bool pause = false;
    bool step = false;
    pfui::Window* activeWindow = nullptr;

    static const uint8_t subWinCount = 2;
    pfui::Window subWindows[subWinCount];  

    sf::RenderWindow window;
};

#endif // EMULATOR_DEBUG_WINDOW_HPP
