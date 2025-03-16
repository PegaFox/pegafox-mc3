#ifndef EMULATOR_DEBUG_WINDOW_HPP
#define EMULATOR_DEBUG_WINDOW_HPP

#include <SFML/Graphics/RenderWindow.hpp>
#include "gui-lib/gui.hpp"

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
      window.create(sf::VideoMode(sf::Vector2u(512, 512)), "Debug Window");
      pause = true;
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
            }
          }
        }

        window.clear(sf::Color::Black);

        if (enabledModes.cpu)
        {
          subWindows[0].draw(window);
        }

        if (enabledModes.ram)
        {
          subWindows[1].draw(window);
        }

        if (enabledModes.vga)
        {
          subWindows[2].draw(window);
        }

        if (enabledModes.tty)
        {
          subWindows[3].draw(window);
        }

        if (enabledModes.hdd)
        {
          subWindows[4].draw(window);
        }

        if (enabledModes.keyboard)
        {
          subWindows[5].draw(window);
        }

        if (enabledModes.mouse)
        {
          subWindows[6].draw(window);
        }

        if (enabledModes.speaker)
        {
          subWindows[7].draw(window);
        }

        window.display();
      }
    }
  private:
    bool pause = false;

    pfui::Window subWindows[8];  

    sf::RenderWindow window;
};

#endif // EMULATOR_DEBUG_WINDOW_HPP
