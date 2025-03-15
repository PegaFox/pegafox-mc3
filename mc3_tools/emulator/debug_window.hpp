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

    void create()
    {
      window.create(sf::VideoMode(sf::Vector2u(512, 512)), "Debug Window");
    }

    void update()
    {
      if (window.isOpen())
      {
        while (window.pollEvent()) {}

        window.clear(sf::Color::Black);

        

        window.display();
      }
    }
  private:
    sf::RenderWindow window;
};
