#ifndef EMULATOR_DEBUG_WINDOW_HPP
#define EMULATOR_DEBUG_WINDOW_HPP

#include <cstring>
#include <iostream>
#include <format>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <map>
#include "gui-lib/include/gui.h"
#include "../disassembler/disassemble_instruction.hpp"
#include "../elf_handler/elf.hpp"
#include "virt_machine.hpp"
#include "emu-utils/ram.hpp"

class DebugWindow;

extern VirtMachine vm;
extern RAM<0xFF00> ram;
extern DebugWindow debugWindow;

class DebugWindow
{
  public:
    struct WindowDivision
    {
      float slicePosition = 0.5f;
      bool verticalSlice = true;
      uint8_t parentIndex = -1;
      uint8_t windowIndex = -1;
      uint8_t child1 = -1;
      uint8_t child2 = -1;
    };

    bool pause = false;

    std::map<uint16_t, SymbolData> symbols;

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

      pfui::GUIElement::drawRect = drawRect;
      pfui::GUIElement::drawLine = drawLine;
      pfui::GUIElement::drawText = drawText;
      pfui::GUIElement::getTextBounds = getTextBounds;
      //pfui::GUIElement::defaultBackgroundColor.a = 128;
      pfui::GUIElement::defaultInteractableColor.a = 1.0f;
    
      windowChoices[0].pos = glm::vec2(-0.48f, -0.935f);
      windowChoices[0].size = glm::vec2(1.0f, 0.1f);

      windowChoices[1].pos = glm::vec2(-0.48f, -0.845f);
      windowChoices[1].size = glm::vec2(1.0f, 0.1f);

      windowTree.emplace_back();
      subWindows[windowTree[addSubWindow(0)].windowIndex].title = "MENU";
      subWindows[windowTree[addSubWindow(0, false, true, 0.5f)].windowIndex].title = "MENU";
    }

    uint8_t addSubWindow(uint8_t nodeIndex, bool beforeCurrent = true, bool verticalSlice = true, float slicePosition = 0.5f)
    {
      uint8_t result = nodeIndex;

      if (windowTree[nodeIndex].windowIndex == (uint8_t)-1)
      {
        if (windowTree[nodeIndex].child1 == (uint8_t)-1 && windowTree[nodeIndex].child2 == (uint8_t)-1)
        {
          windowTree[nodeIndex].windowIndex = subWindows.size();
        } else
        {
          return result;
        }
      } else
      {
        windowTree.emplace_back();
        windowTree.back().parentIndex = nodeIndex;
        windowTree.back().windowIndex = windowTree[nodeIndex].windowIndex;

        windowTree.emplace_back();
        windowTree.back().parentIndex = nodeIndex;
        windowTree.back().windowIndex = subWindows.size();
        result = windowTree.size()-1;

        windowTree[nodeIndex].windowIndex = -1;

        if (beforeCurrent)
        {
          windowTree[nodeIndex].child1 = windowTree.size()-1;
          windowTree[nodeIndex].child2 = windowTree.size()-2;
        } else
        {
          windowTree[nodeIndex].child1 = windowTree.size()-2;
          windowTree[nodeIndex].child2 = windowTree.size()-1;
        }
      }

      windowTree[nodeIndex].slicePosition = slicePosition;
      windowTree[nodeIndex].verticalSlice = verticalSlice;

      subWindows.emplace_back();
      subWindows.back().title = "MENU";
      subWindows.back().hasTitlebar = false;// why does this break scrolling? TODO: Switch to tiling windows
      subWindows.back().isDraggable = false;
      subWindows.back().resizeable = true;
      subWindows.back().pos = subWindowArea.position;
      subWindows.back().size = subWindowArea.size;

      subWindows.back().addChild(&windowChoices[0], -1, false);
      subWindows.back().addChild(&windowChoices[0], -1, false);
      subWindows.back().addChild(&windowChoices[1], -1, false);
      subWindows.back().addChild(&windowChoices[1], -1, false);

      pfui::Paragraph* winData = new pfui::Paragraph;
      subWindows.back().addChild(winData);
      //winData->scrollable = true;
      winData->wrapMode = pfui::Paragraph::WrapMode::None;
      winData->size = glm::vec2(0.09f);
      winData->pos = glm::vec2(-0.98f);
      winData->textHeight = 0.09f;
      winData->color = pfui::Color(1.0f);
      winData->text = "CPU\nRAM\n";

      //std::cout << "add window\n";
    
      return result;
    }

    void removeNode(uint8_t nodeIndex)
    {
      // try starting out as reference then setting to stack value just before removal
      WindowDivision node = windowTree[nodeIndex];
      if (node.windowIndex == (uint8_t)-1)
      {
        if (node.child1 == (uint8_t)-1 && node.child2 == (uint8_t)-1)
        {
          return;
        } else
        {
          // parent seperation must be interleaved in case swap removing child1 changes the position of child2
          windowTree[windowTree[nodeIndex].child1].parentIndex = -1;
          removeNode(windowTree[nodeIndex].child1);

          windowTree[windowTree[nodeIndex].child2].parentIndex = -1;
          removeNode(windowTree[nodeIndex].child2);
        }
      } else
      {
        subWindows[node.windowIndex].title = "";
        //subWindows[node.windowIndex].~Window();
      }

      windowTree[nodeIndex] = WindowDivision{};

      if (node.parentIndex != (uint8_t)-1)
      {
        WindowDivision* parent = &windowTree[node.parentIndex];

        uint8_t otherChild = nodeIndex == parent->child1 ? parent->child2 : parent->child1;

        if (heldNode == nodeIndex)
        {
          heldNode = otherChild;
        }

        windowTree[otherChild].parentIndex = parent->parentIndex;
        
        if (parent->parentIndex != (uint8_t)-1)
        {
          (node.parentIndex == windowTree[parent->parentIndex].child1 ? windowTree[parent->parentIndex].child1 : windowTree[parent->parentIndex].child2) = otherChild;
        }
        
        *parent = WindowDivision{};

        if (heldNode == node.parentIndex)
        {
          heldNode = -1;
        }
      }

      //std::cout << "remove node\n";
    }

    void cleanTree()
    {
      if (windowTree.size() < 2)
      {
        return;
      }

      for (uint8_t n = windowTree.size()-1; n != (uint8_t)-1; n--)
      {
        if (windowTree[n].parentIndex == (uint8_t)-1 && windowTree[n].child1 == (uint8_t)-1 && windowTree[n].child2 == (uint8_t)-1 && windowTree[n].windowIndex == (uint8_t)-1)
        {
          if (heldNode == windowTree.size()-1)
          {
            heldNode = &windowTree[n] - windowTree.data();
          }

          windowTree[n] = windowTree.back();

          if (windowTree[n].parentIndex != (uint8_t)-1)
          {
            WindowDivision* parent = &windowTree[windowTree[n].parentIndex];

            (parent->child1 == windowTree.size()-1 ? parent->child1 : parent->child2) = &windowTree[n] - windowTree.data();
          }
          windowTree.pop_back();
        }
      }
    }

    WindowDivision* findSubWindow(glm::vec2 location)
    {
      pfui::Rect windowBounds{glm::vec2(-1.0f), glm::vec2(2.0f)};

      WindowDivision* currentNode = &windowTree[0];
      while (currentNode->windowIndex == (uint8_t)-1)
      {
        pfui::Rect child1Bounds;
        pfui::Rect child2Bounds;

        if (currentNode->verticalSlice)
        {
          child1Bounds = {windowBounds.position, glm::vec2(windowBounds.size.x * currentNode->slicePosition, windowBounds.size.y)};
          child2Bounds = {glm::vec2(windowBounds.position.x + child1Bounds.size.x, windowBounds.position.y), glm::vec2(windowBounds.size.x * (1.0f-currentNode->slicePosition), windowBounds.size.y)};
        } else
        {
          child1Bounds = {windowBounds.position, glm::vec2(windowBounds.size.x, windowBounds.size.y * currentNode->slicePosition)};
          child2Bounds = {glm::vec2(windowBounds.position.x, windowBounds.position.y + child1Bounds.size.y), glm::vec2(windowBounds.size.x, windowBounds.size.y * (1.0f-currentNode->slicePosition))};
        }

        if (child1Bounds.contains(location))
        {
          windowBounds = child1Bounds;

          currentNode = &windowTree[currentNode->child1];
        } else
        {
          windowBounds = child2Bounds;

          currentNode = &windowTree[currentNode->child2];
        }
      }

      return currentNode;
    }

    uint8_t findRootNode()
    {
      uint8_t currentIndex = 0;

      while (windowTree[currentIndex].parentIndex != (uint8_t)-1)
      {
        currentIndex = windowTree[currentIndex].parentIndex;
      }

      return currentIndex;
    }

    void updateSubWindowTransforms(uint8_t rootIndex, glm::mat3 rootTransform)
    {
      uint8_t stack[16], stackTop = 0;
      glm::mat3 transforms[16];

      stack[stackTop] = rootIndex;
      transforms[stackTop++] = rootTransform;

      while (stackTop > 0)
      {
        stackTop--;

        WindowDivision node = windowTree[stack[stackTop]];
        glm::mat3 transform = transforms[stackTop];

        if (node.windowIndex == (uint8_t)-1)
        {
          if (node.child1 != (uint8_t)-1)
          {
            stack[stackTop] = node.child1;
            transforms[stackTop] = transform;

            if (node.verticalSlice)
            {
              transforms[stackTop][0][0] *= node.slicePosition;
              transforms[stackTop][2][0] -= transform[0][0] * (1.0f-node.slicePosition);
            } else
            {
              transforms[stackTop][1][1] *= node.slicePosition;
              transforms[stackTop][2][1] -= transform[1][1] * (1.0f-node.slicePosition);
            }

            stackTop++;
          }

          if (node.child2 != (uint8_t)-1)
          {
            stack[stackTop] = node.child2;
            transforms[stackTop] = transform;

            if (node.verticalSlice)
            {
              transforms[stackTop][0][0] *= 1.0f - node.slicePosition;
              transforms[stackTop][2][0] += transform[0][0] * node.slicePosition;
            } else
            {
              transforms[stackTop][1][1] *= 1.0f - node.slicePosition;
              transforms[stackTop][2][1] += transform[1][1] * node.slicePosition;
            }

            stackTop++;
          }
        } else
        {
          subWindows[node.windowIndex].transform = transform;
          subWindows[node.windowIndex].pos = subWindowArea.position;
          subWindows[node.windowIndex].size = subWindowArea.size;
        }
      }
    }

    void update()
    {
      if (window.isOpen())
      {
        glm::vec2 halfWinSize(window.getSize().x * 0.5f, window.getSize().y * 0.5f);

        while (std::optional<sf::Event> event = window.pollEvent())
        {
          if (const sf::Event::KeyPressed* key = event->getIf<sf::Event::KeyPressed>())
          {
            if (key->code == sf::Keyboard::Key::PageUp)
            {
              uint32_t* drawStart = &((pfui::Paragraph*)(subWindows[windowTree[heldNode].windowIndex])[0])->drawStart;

              if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
              {
                *drawStart = 0;
              } else
              {
                *drawStart = glm::max(int(*drawStart)-22, 0);
              }
            } else if (key->code == sf::Keyboard::Key::PageDown)
            {
              uint32_t* drawStart = &((pfui::Paragraph*)(subWindows[windowTree[heldNode].windowIndex])[0])->drawStart;

              if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
              {
                *drawStart = -1;
              } else
              {
                *drawStart = glm::max(int(*drawStart)+22, 0);
              }
            }
          } else if (const sf::Event::KeyReleased* key = event->getIf<sf::Event::KeyReleased>())
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

            glm::vec2 mPos(scroll->position.x, scroll->position.y);

            float delta = scroll->delta;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            {
              delta *= 16;
            }
            if (heldNode != (uint8_t)-1 && subWindows[windowTree[heldNode].windowIndex].getGlobalBounds().contains(mPos/halfWinSize - 1.0f))
            {
              uint32_t* drawStart = &((pfui::Paragraph*)(subWindows[windowTree[heldNode].windowIndex])[0])->drawStart;
              *drawStart = glm::max(float(*drawStart)-delta, 0.0f);
            } else
            {
              uint32_t* drawStart = &((pfui::Paragraph*)(subWindows[findSubWindow(mPos/halfWinSize - 1.0f)->windowIndex])[0])->drawStart;
              *drawStart = glm::max(float(*drawStart)-delta, 0.0f);
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

            glm::vec2 mPos(button->position.x, button->position.y);

            heldNode = findSubWindow(mPos/halfWinSize - 1.0f) - windowTree.data();
            /*if (!activeWindow || !activeWindow->getGlobalBounds().contains(mPos/halfWinSize - 1.0f))
            {
              for (uint8_t w = 0; w < subWinCount; w++)
              {
                if (subWindows[w].getGlobalBounds().contains(glm::vec2(button->position.x/512.0f - 1.0f, button->position.y/256.0f - 1.0f)))
                {
                  activeWindow = subWindows+w;
                  break;
                }
              }
            }*/
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
            glm::vec2 mPos(movement->position.x, movement->position.y);
            //std::cout << mPos.x/halfWinSize.x - 1.0f << " " << mPos.y/halfWinSize.y - 1.0f << "\n";

            pfui::GUIElement::updateCursor(mPos/halfWinSize - 1.0f);

            if (heldNode != (uint8_t)-1 && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            {
              //WindowDivision* selected = findSubWindow(mPos/halfWinSize - 1.0f);
              WindowDivision* selected = &windowTree[heldNode];
              pfui::Window& selectedWindow = subWindows[selected->windowIndex];

              if (selectedWindow.size != subWindowArea.size)
              {
                bool verticalSlice = selectedWindow.size.x != subWindowArea.size.x;
                bool firstChild = (selectedWindow.pos[!verticalSlice] - subWindowArea.position[!verticalSlice] > 0.0f) == (selectedWindow.size[!verticalSlice] - subWindowArea.size[!verticalSlice] > 0.0f);

                while (selected->parentIndex != (uint8_t)-1)
                {
                  WindowDivision* parent = &windowTree[selected->parentIndex];

                  if (parent->verticalSlice == verticalSlice && (selected - windowTree.data() == parent->child1) == firstChild)
                  {
                    float adjustedSlicePosition = firstChild ? parent->slicePosition : 1.0f - parent->slicePosition;

                    parent->slicePosition = (selectedWindow.size[!parent->verticalSlice] / (subWindowArea.size[!parent->verticalSlice] / adjustedSlicePosition));
                    if (!firstChild)
                    {
                      parent->slicePosition = 1.0f - parent->slicePosition;
                    }

                    //std::cout << parent->slicePosition << '\n';
                    if (parent->slicePosition > 0.999f)
                    {
                      removeNode(parent->child2);
                      cleanTree();
                    } else if (parent->slicePosition < 0.001f)
                    {
                      removeNode(parent->child1);
                      cleanTree();
                    }

                    selectedWindow.pos = subWindowArea.position;
                    selectedWindow.size = subWindowArea.size;
                    break;
                  }

                  selected = parent;
                }

                if (selectedWindow.size != subWindowArea.size)
                {
                  //std::cout << "edge pull\n";
                  selected = &windowTree[heldNode];
                  addSubWindow(heldNode, !firstChild, verticalSlice, 0.001f + 0.998f*firstChild);

                  // these res
                  heldNode = firstChild ? windowTree[heldNode].child1 : windowTree[heldNode].child2;
                  pfui::GUIElement::updateMouseButtons({
                    false,
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Right),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra1),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra2),
                  });
                  pfui::GUIElement::updateMouseButtons({
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Right),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra1),
                    sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra2),
                  });
                }
              }
            }
          }
        }

        /*
        std::cout << "[\n";
        for (WindowDivision& node: windowTree)
        {
          std::cout << "[" << (&node - windowTree.data()) << "]: {\n";

          if (node.windowIndex == (uint8_t)-1) std::cout << "    slicePosition: " << node.slicePosition << ",\n";
          if (node.windowIndex == (uint8_t)-1) std::cout << "    verticalSlice: " << (node.verticalSlice ? "true" : "false") << ",\n";
          std::cout << "    parentIndex:   " << (int)node.parentIndex << ",\n";
          if (node.windowIndex != (uint8_t)-1) std::cout << "    windowIndex:   " << (int)node.windowIndex << ",\n";
          if (node.child1 != (uint8_t)-1) std::cout << "    child1:        " << (int)node.child1 << ",\n";
          if (node.child2 != (uint8_t)-1) std::cout << "    child2:        " << (int)node.child2 << ",\n";

          std::cout << "  },\n";
        }
        std::cout << "]\n";
        */

        updateSubWindowTransforms(findRootNode(), glm::mat3(
          glm::vec3(1.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 1.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, 1.0f)
        ));

        window.clear(sf::Color::Black);

        for (pfui::Window& win: subWindows)
        {
          if (!win.title.empty())
          {
            if (win.title == "CPU")
            {
              std::string& winData = ((pfui::Paragraph*)win[win.childCount()-1])->text;
              winData = "";

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
            } else if (win.title == "RAM")
            {
              std::string& winData = ((pfui::Paragraph*)win[win.childCount()-1])->text;
              winData = "";

              uint32_t* drawStart = &((pfui::Paragraph*)(win)[win.childCount()-1])->drawStart;

              std::map<uint16_t, SymbolData>::iterator currentVar =
                symbols.end();
              if (!symbols.empty())
              {
                std::map<uint16_t, SymbolData>::iterator lastSym =
                  --symbols.upper_bound(*drawStart);
                if (
                  lastSym->second.type == SymbolData::Variable &&
                  lastSym->first + lastSym->second.size > *drawStart)
                {
                  currentVar = lastSym;
                }
              }

              *drawStart = glm::min(sizeof(ram.memory)-22, (size_t)*drawStart);
              for (uint32_t b = *drawStart; b < *drawStart+22; b++)
              {
                if (currentVar != symbols.end())
                {
                  if (b >= currentVar->first + currentVar->second.size)
                  {
                    if (symbols.contains(b))
                    {
                      winData += "} ";
                    } else
                    {
                      winData += "}\n";
                      currentVar = symbols.end();
                    }
                  }
                }

                if (symbols.contains(b))
                {
                  winData += symbols[b].name;

                  if (symbols[b].type == SymbolData::Variable)
                  {
                    winData += " {\n";
                    currentVar = symbols.find(b);
                  } else if (symbols[b].type == SymbolData::Label)
                  {
                    winData += ":\n";
                  }
                }

                winData += std::format("{:04X}:{:02X}", uint16_t(b), ram.memory[b]);
                if (b%2 && currentVar == symbols.end())
                {
                  winData += (b == vm.pc+1 ? ">" : " ") + disassembleInstruction(ram.memory[b-1], ram.memory[b]);
                }
                winData += "\n";
              }
            }

            /*std::cout << win.transform[0][0] << "\t" << win.transform[0][1] << "\t" << win.transform[0][2] << "\n";
            std::cout << win.transform[1][0] << "\t" << win.transform[1][1] << "\t" << win.transform[1][2] << "\n";
            std::cout << win.transform[2][0] << "\t" << win.transform[2][1] << "\t" << win.transform[2][2] << "\n\n";*/
            win.size = glm::vec2(2.0f) - (glm::vec2(2.0f)-subWindowArea.size)/glm::vec2(win.transform[0][0], win.transform[1][1]);

            win.draw();

            win.size = subWindowArea.size;

            if (win.title == "MENU")
            {
              for (uint8_t b = 0; b < sizeof(windowChoices)/sizeof(windowChoices[0]); b++)
              {
                if (windowChoices[b].isPressed())
                {
                  win.title = winNames[b];
                  
                  while (win.childCount() > 1)
                  {
                    win.removeChild(0);
                  }
                }
              }
            }
          }
        }
        //std::cout << "\n";

        window.display();
      }
    }
  private:
    bool step = false;

    static const pfui::Rect subWindowArea;
    std::vector<WindowDivision> windowTree;
    std::vector<pfui::Window> subWindows;  
    uint8_t heldNode = -1;

    pfui::Button windowChoices[2];
    static constexpr std::string_view winNames[8] = {
      "CPU",
      "RAM",
      "VGA",
      "TTY",
      "HDD",
      "KEYBOARD",
      "MOUSE",
      "SPEAKER"
    };

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

const pfui::Rect DebugWindow::subWindowArea = pfui::Rect{glm::vec2(0.0f), glm::vec2(1.995f)};

#endif // EMULATOR_DEBUG_WINDOW_HPP
