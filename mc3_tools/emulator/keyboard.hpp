// uses two bytes
// read byte 1: pops and returns the first scancode in the event queue, if the event queue is empty, the response is zero.
// write byte 1: sends an instruction to the keyboard, if the instruction is a request, the response is stored at byte 2.
// read byte 2: returns the response to the last request or zero if there is no response.
// write byte 2: sends an instruction operand to the keyboard.
class Keyboard: public ExternalDevice<uint16_t>
{
  public:
    Keyboard()
    {
      
    }

    virtual uint8_t read(uint16_t position)
    {
      if (position == 0)
      {
        if (eventQueue.empty())
        {
          return 0;
        }

        eventQueue.pop_front();

        if (eventQueue.empty())
        {
          return 0;
        }

        return eventQueue.front();
      }

      uint8_t value = response;
      response = 0;
      return value;
    }

    virtual void write(uint16_t position, uint8_t value)
    {
      if (position == 0)
      {
        command = value;
        switch (value)
        {
          case 0xEE: // echo
            response = 0xEE;
            break;
          case 0xF2: // identify
            response = 0xAB;
            break;
          case 0xF3: // repeat rate

            break;
          case 0xF4: // enable scanning
            scanning = true;
            response = 0xFA;
            break;
          case 0xF5: // disable scanning
            scanning = false;
            response = 0xFA;
            break;
          case 0xF6: // reset settings

            break;
          case 0xFE: // resend
            response = eventQueue.front();
            break;
          case 0xFF: // reset
            command = 0;
            response = 0;

            scanning = false;

            eventQueue.clear();
            break;
        }
      } else
      {
        if (command == 0xF3)
        {
          
        }
      }
    }

    bool update()
    {
      if (scanning)
      {
        bool eventAdded = false;
  
        for (int i = 0; i < sf::Keyboard::KeyCount; i++)
        {
          bool state = sf::Keyboard::isKeyPressed((sf::Keyboard::Key)i);
          if (state != keyStates[i])
          {
            keyStates[i] = state;
            if (!state)
            {
              eventQueue.push_back(0xF0);
            }
  
            if (scancodes[i] >> 8)
            {
              eventQueue.push_back(scancodes[i] >> 8);
            }
  
            eventQueue.push_back(scancodes[i] & 0xFF);
  
            eventAdded = true;
          }
        }
  
        return eventAdded;
      }
      return false;
    }
  private:
    uint8_t command = 0;
    uint8_t response = 0;

    bool scanning = false;

    static const std::array<uint16_t, sf::Keyboard::KeyCount> scancodes;

    std::array<bool, sf::Keyboard::KeyCount> keyStates;

    std::deque<uint8_t> eventQueue;
};

const std::array<uint16_t, sf::Keyboard::KeyCount> Keyboard::scancodes = {
  0x1C,              // The A key
  0x32,              // The B key
  0x21,              // The C key
  0x23,              // The D key
  0x24,              // The E key
  0x2B,              // The F key
  0x34,              // The G key
  0x33,              // The H key
  0x43,              // The I key
  0x3B,              // The J key
  0x42,              // The K key
  0x4B,              // The L key
  0x3A,              // The M key
  0x31,              // The N key
  0x44,              // The O key
  0x4D,              // The P key
  0x15,              // The Q key
  0x2D,              // The R key
  0x1B,              // The S key
  0x2C,              // The T key
  0x3C,              // The U key
  0x2A,              // The V key
  0x1D,              // The W key
  0x22,              // The X key
  0x35,              // The Y key
  0x1A,              // The Z key
  0x45,              // The 0 key
  0x16,              // The 1 key
  0x1E,              // The 2 key
  0x26,              // The 3 key
  0x25,              // The 4 key
  0x2E,              // The 5 key
  0x36,              // The 6 key
  0x3D,              // The 7 key
  0x3E,              // The 8 key
  0x46,              // The 9 key
  0x76,              // The Escape key
  0x14,              // The left Control key
  0x12,              // The left Shift key
  0x11,              // The left Alt key
  0xE01F,            // The left OS specific key: window (Windows and Linux), apple (macOS), ...
  0xE014,            // The right Control key
  0x59,              // The right Shift key
  0xE011,            // The right Alt key
  0xE027,            // The right OS specific key: window (Windows and Linux), apple (macOS), ...
  0xE02F,            // The Menu key
  0x54,              // The [ key
  0x5B,              // The ] key
  0x4C,              // The ; key
  0x41,              // The , key
  0x49,              // The . key
  0x52,              // The ' key
  0x4A,              // The / key
  0x5D,              // The \ key
  0x0E,              // The ` key
  0x55,              // The = key
  0x4E,              // The - key (hyphen)
  0x29,              // The Space key
  0x5A,              // The Enter/Return keys
  0x66,              // The Backspace key
  0x0D,              // The Tabulation key
  0xE07D,            // The Page up key
  0xE07A,            // The Page down key
  0xE069,            // The End key
  0xE06C,            // The Home key
  0xE070,            // The Insert key
  0xE071,            // The Delete key
  0x79,              // The + key
  0x7B,              // The - key (minus, usually from numpad)
  0x7C,              // The * key
  0xE04A,            // The / key
  0xE06B,            // Left arrow
  0xE074,            // Right arrow
  0xE075,            // Up arrow
  0xE07A,            // Down arrow
  0x70,              // The numpad 0 key
  0x69,              // The numpad 1 key
  0x72,              // The numpad 2 key
  0x7A,              // The numpad 3 key
  0x6B,              // The numpad 4 key
  0x73,              // The numpad 5 key
  0x74,              // The numpad 6 key
  0x6C,              // The numpad 7 key
  0x75,              // The numpad 8 key
  0x7D,              // The numpad 9 key
  0x05,              // The F1 key
  0x06,              // The F2 key
  0x04,              // The F3 key
  0x0C,              // The F4 key
  0x03,              // The F5 key
  0x0B,              // The F6 key
  0x83,              // The F7 key
  0x0A,              // The F8 key
  0x01,              // The F9 key
  0x09,              // The F10 key
  0x78,              // The F11 key
  0x07,              // The F12 key
  0x00,              // The F13 key
  0x00,              // The F14 key
  0x00,              // The F15 key
  (uint16_t)0xE11477E1F014F077 // The Pause key
};