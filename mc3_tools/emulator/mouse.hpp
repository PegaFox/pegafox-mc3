// uses four bytes
// byte 1: zero, zero, Y sign, X sign, one, middle button, right button, left button
// byte 2: X delta
// byte 3: Y delta
// byte 4: zero, zero, button 5, button 4, scroll wheel
class Mouse: public ExternalDevice<uint16_t>
{
  public:
    Mouse()
    {
      mPos = sf::Mouse::getPosition();
    }

    virtual uint8_t read(uint16_t position)
    {
      switch (position)
      {
        case 0:
          return
            ((mDelta.y < 0) << 5 ) |
            ((mDelta.x < 0) << 4) |
            0x08 |
            (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle) << 2) |
            (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) << 1) |
            sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
          break;
        case 1:
          return mDelta.x & 0xFF;
          break;
        case 2:
          return mDelta.y & 0xFF;
          break;
        case 3:
          return (sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra2) << 5) | (sf::Mouse::isButtonPressed(sf::Mouse::Button::Extra1) << 4);
          break;
      }

      return 0;
    }

    virtual void write(uint16_t position, uint8_t value)
    {
      
    }

    // updates position delta values
    void update()
    {
      sf::Vector2i newPos = sf::Mouse::getPosition();

      mDelta = newPos - mPos;

      mPos = newPos;
    }
  private:
    sf::Vector2i mPos;
    sf::Vector2i mDelta = sf::Vector2i(0, 0);
};