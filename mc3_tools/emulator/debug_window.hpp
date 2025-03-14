class DebugWindow
{
  public:
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