#ifndef EMULATOR_DEBUG_WINDOW_HPP
#define EMULATOR_DEBUG_WINDOW_HPP

#include <iostream>
#include <format>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include "gui-lib/include/gui.h"
#include "../disassembler/disassemble_instruction.hpp"
#include "virt_machine.hpp"
#include "emu-utils/ram.hpp"

class DebugWindow;

extern VirtMachine vm;
extern RAM<0xFF00> ram;
extern DebugWindow debugWindow;

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

      if (!font.openFromFile(/*"3270NerdFontMono-Regular.ttf"*/"PublicPixel.ttf"))
      {
        std::clog << "Warning: debug window failed to load default font, text rendering may not work correctly\n";
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

      pfui::GUIElement::drawRect = drawRect;
      pfui::GUIElement::drawLine = drawLine;
      pfui::GUIElement::drawText = drawText;
      pfui::GUIElement::getTextBounds = getTextBounds;

      for (uint8_t w = 0; w < subWinCount; w++)
      {
        subWindows[w].title = winNames[w];
        subWindows[w].size = glm::vec2(1.0f, 1.93f);
        subWindows[w].pos = glm::vec2(-0.5f + w, 0.035f);

        pfui::Paragraph* winData = new pfui::Paragraph;
        subWindows[w].addChild(winData);
        //winData->scrollable = true;
        winData->wrapMode = pfui::Paragraph::WrapMode::None;
        winData->size = glm::vec2(0.09f);
        winData->pos = glm::vec2(-0.98f);
        winData->textHeight = 0.09f;
        winData->color = pfui::Color(1.0f);
      }
    }

    void update()
    {
      if (window.isOpen())
      {
        while (std::optional<sf::Event> event = window.pollEvent())
        {
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
            pfui::GUIElement::updateScrollWheel(scroll->delta);

            float delta = scroll->delta;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            {
              delta *= 16;
            }
            if (activeWindow && activeWindow->getGlobalBounds().contains(glm::vec2(scroll->position.x/512.0f - 1.0f, scroll->position.y/256.0f - 1.0f)))
            {
              uint32_t* drawStart = &((pfui::Paragraph*)(*activeWindow)[0])->drawStart;
              *drawStart = glm::max(float(*drawStart)-delta, 0.0f);
            } else
            {
              for (uint8_t w = 0; w < subWinCount; w++)
              {
                if (subWindows[w].getGlobalBounds().contains(glm::vec2(scroll->position.x/512.0f - 1.0f, scroll->position.y/256.0f - 1.0f)))
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
            pfui::GUIElement::updateMouseButtons({
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Right),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra1),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra2),
            });

            if (!activeWindow || !activeWindow->getGlobalBounds().contains(glm::vec2(button->position.x/512.0f - 1.0f, button->position.y/256.0f - 1.0f)))
            {
              for (uint8_t w = 0; w < subWinCount; w++)
              {
                if (subWindows[w].getGlobalBounds().contains(glm::vec2(button->position.x/512.0f - 1.0f, button->position.y/256.0f - 1.0f)))
                {
                  activeWindow = subWindows+w;
                  break;
                }
              }
            }
          } else if (const sf::Event::MouseButtonReleased* button = event->getIf<sf::Event::MouseButtonReleased>())
          {
            pfui::GUIElement::updateMouseButtons({
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Right),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra1),
              sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra2),
            });
          } else if (const sf::Event::MouseMoved* movement = event->getIf<sf::Event::MouseMoved>())
          {
            pfui::GUIElement::updateCursor(glm::vec2(movement->position.x/512.0f - 1.0f, movement->position.y/256.0f - 1.0f));
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
          
          ((pfui::Paragraph*)subWindows[0][0])->text = winData;
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

          ((pfui::Paragraph*)subWindows[1][0])->text = winData;
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
            subWindows[w].draw();
          }
        }

        if (activeWindow)
        {
          activeWindow->draw();
        }

        window.display();
      }
    }
  private:
    bool step = false;
    pfui::Window* activeWindow = nullptr;

    static const uint8_t subWinCount = 2;
    pfui::Window subWindows[subWinCount];  

    sf::Font font;
    sf::RenderWindow window;

    static void drawRect(pfui::Rect rect, pfui::Color color)
    {
      sf::RectangleShape drawRect(sf::Vector2f(rect.size.x * 512.0f, rect.size.y * 256.0f));

      drawRect.setPosition(sf::Vector2f(rect.position.x*512.0f + 512.0f, rect.position.y*256.0f + 256.0f));

      drawRect.setFillColor(sf::Color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f));

      debugWindow.window.draw(drawRect);
    }

    static void drawLine(glm::vec2 position1, glm::vec2 position2, pfui::Color color)
    {
      sf::VertexArray drawLine(sf::PrimitiveType::Lines, 2);

      drawLine[0] = sf::Vertex{sf::Vector2f(position1.x*512.0f + 512.0f, position1.y*256.0f + 256.0f), sf::Color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f)};
      drawLine[1] = sf::Vertex{sf::Vector2f(position2.x*512.0f + 512.0f, position2.y*256.0f + 256.0f), sf::Color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f)};

      debugWindow.window.draw(drawLine);
    }

    static void drawText(pfui::FontID font, const char* text, glm::vec2 pos, float height, pfui::Color color)
    {
      sf::Text drawText(debugWindow.font);

      drawText.setString(text);

      drawText.setPosition(sf::Vector2f(pos.x*512.0f + 512.0f, pos.y*256.0f + 256.0f));

      drawText.setCharacterSize(height*256.0f);

      drawText.setFillColor(sf::Color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f));

      debugWindow.window.draw(drawText);
    }

    static pfui::Rect getTextBounds(pfui::FontID font, const char* text, glm::vec2 pos, float height)
    {
      sf::Text drawText(debugWindow.font);

      drawText.setString(text);

      drawText.setPosition(sf::Vector2f(pos.x, pos.y));

      drawText.setCharacterSize(height*256.0f);
      drawText.setScale(sf::Vector2f(1.0f / 512.0f, 1.0f / 256.0f));

      sf::FloatRect bounds = drawText.getGlobalBounds();

      return pfui::Rect{glm::vec2(bounds.position.x, bounds.position.y), glm::vec2(bounds.size.x, bounds.size.y)};
    }
};

#endif // EMULATOR_DEBUG_WINDOW_HPP
