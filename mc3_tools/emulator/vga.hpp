// uses six bytes
// write byte 1: send opcode. This should be done after setting any parameters.
// write bytes 2-4: set parameters/set vram access pointer.
// write bytes 5-6: set parameters/write to video memory.
// read bytes 2-4: read vram access pointer.
// read bytes 5-6: read from video memory.
class VGA: public ExternalDevice<uint16_t>
{
  public:
    virtual uint8_t read(uint16_t position)
    {
      if (position > 0)
      {
        if (position < 4)
        {
          return framebufferPointer >> ((position-1)*8);
        } else
        {
          sf::Vector2u pos((framebufferPointer/bytesPerPixel) % framebuffer.getSize().x, (framebufferPointer/bytesPerPixel) / framebuffer.getSize().x);
          switch (bytesPerPixel)
          {
            case 1: {
              sf::Color currentColor = framebuffer.getPixel(pos);
              return (currentColor.b << 5) | (currentColor.g << 2) | currentColor.r;
              break;
            } case 2:
              if (framebufferPointer % bytesPerPixel)
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return (currentColor.a) | (currentColor.b << 2) | (currentColor.g >> 6);
              } else
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return (currentColor.g << 5) | currentColor.r;
              }
              break;
            case 3:
              if (framebufferPointer % bytesPerPixel == 0)
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.r;
              } else if (framebufferPointer % bytesPerPixel == 1)
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.g;
              } else
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.b;
              }
              break;
            case 4:
              if (framebufferPointer % bytesPerPixel == 0)
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.r;
              } else if (framebufferPointer % bytesPerPixel == 1)
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.g;
              } else if (framebufferPointer % bytesPerPixel == 2)
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.b;
              } else
              {
                sf::Color currentColor = framebuffer.getPixel(pos);
                return currentColor.a;
              }
              break;
          }
        }
      }

      return 0;
    }

    virtual void write(uint16_t position, uint8_t value)
    {
      if (position > 0)
      {
        parameters[position-1] = value;

        if (framebufferAccess)
        {
          if (position < 4)
          {
            framebufferPointer = (framebufferPointer & ~(0xFF << ((position-1)*8))) | (uint32_t)value << ((position-1)*8);
          } else
          {
            sf::Vector2u pos((framebufferPointer/bytesPerPixel) % framebuffer.getSize().x, (framebufferPointer/bytesPerPixel) / framebuffer.getSize().x);
            switch (bytesPerPixel)
            {
              case 1:
                framebuffer.setPixel(pos, sf::Color(value << 6, (value << 3) & 0xE0, value & 0xE0));
                break;
              case 2:
                if (framebufferPointer % bytesPerPixel)
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.g = (currentColor.g & 0x3F) | ((value & 0x3) << 6);
                  currentColor.b = (value & 0x7C) >> 2;
                  currentColor.a = value & 0x80;
                  framebuffer.setPixel(pos, currentColor);
                } else
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.r = value & 0x1F;
                  currentColor.g = (currentColor.g & 0xF7) | ((value & 0xE0) >> 5);
                  framebuffer.setPixel(pos, currentColor);
                }
                break;
              case 3:
                if (framebufferPointer % bytesPerPixel == 0)
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.r = value;
                  framebuffer.setPixel(pos, currentColor);
                } else if (framebufferPointer % bytesPerPixel == 1)
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.g = value;
                  framebuffer.setPixel(pos, currentColor);
                } else
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.b = value;
                  framebuffer.setPixel(pos, currentColor);
                }
                break;
              case 4:
                if (framebufferPointer % bytesPerPixel == 0)
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.r = value;
                  framebuffer.setPixel(pos, currentColor);
                } else if (framebufferPointer % bytesPerPixel == 1)
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.g = value;
                  framebuffer.setPixel(pos, currentColor);
                } else if (framebufferPointer % bytesPerPixel == 2)
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.b = value;
                  framebuffer.setPixel(pos, currentColor);
                } else
                {
                  sf::Color currentColor = framebuffer.getPixel(pos);
                  currentColor.a = value;
                  framebuffer.setPixel(pos, currentColor);
                }
                break;
            }
          }
        }
      } else
      {
        if (value == 0)
        {
          framebufferAccess = true;
        } else
        {
          framebufferAccess = false;
        }

        if (value == 1)
        {
          window.create(sf::VideoMode(sf::Vector2u(((uint16_t)parameters[1] << 8) | (uint16_t)parameters[0], ((uint16_t)parameters[3] << 8) | (uint16_t)parameters[2])), "PF2");
          framebuffer.resize(window.getSize());
          bytesPerPixel = parameters[4];
        }
      }
    }

    void update()
    {
      if (window.isOpen())
      {
        while (window.pollEvent()) {}

        window.clear(sf::Color::Black);

        sf::Texture tex(framebuffer);
        window.draw(sf::Sprite(tex));

        window.display();
      }
    }
  private:
    bool framebufferAccess = false;

    uint32_t framebufferPointer = 0;

    uint8_t bytesPerPixel = 0;

    uint8_t parameters[5];
  
    sf::Image framebuffer;

    sf::RenderWindow window;
};