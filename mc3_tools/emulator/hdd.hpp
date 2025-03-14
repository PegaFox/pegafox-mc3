// uses seven bytes
// read/write bytes 1-4: access the selected block.
// read/write byte 5: access the selected word.
// read/write bytes 6-7: access the byte at the specified offset in the selected block.
class HDD: public ExternalDevice<uint16_t>
{
  public:
    HDD(const std::string& filename)
    {
      std::filebuf drive;
      drive.open(filename, std::ios::in | std::ios::binary);

      if (drive.is_open())
      {
        data.resize(drive.in_avail() >> 9);

        drive.sgetn((char*)data.data(), data.size() << 9);
        drive.close();
      }
    }

    virtual uint8_t read(uint16_t position)
    {
      switch (position)
      {
        case 0:
          return selectedBlock & 0xFF;
          break;
        case 1:
          return (selectedBlock >> 8) & 0xFF;
          break;
        case 2:
          return (selectedBlock >> 16) & 0xFF;
          break;
        case 3:
          return selectedBlock >> 24;
          break;
        case 4:
          return wordOffset;
          break;
        case 6:
          return data[selectedBlock][(uint16_t)wordOffset << 1];
          break;
        case 7:
          return data[selectedBlock][((uint16_t)wordOffset << 1) + 1];
          break;
      }
      return 0;
    }

    virtual void write(uint16_t position, uint8_t value)
    {
      switch (position)
      {
        case 0:
          selectedBlock = (selectedBlock & 0xFFFFFF00) | value;
          break;
        case 1:
          selectedBlock = (selectedBlock & 0xFFFF00FF) | (value << 8);
          break;
        case 2:
          selectedBlock = (selectedBlock & 0xFF00FFFF) | (value << 16);
          break;
        case 3:
          selectedBlock = (selectedBlock & 0x00FFFFFF) | (value << 24);
          break;
        case 4:
          wordOffset = value;
          break;
        case 6:
          data[selectedBlock][(uint16_t)wordOffset << 1] = value;
          break;
        case 7:
          data[selectedBlock][((uint16_t)wordOffset << 1) + 1] = value;
          break;
      }
    }
  private:
    uint32_t selectedBlock = 0;
    uint8_t wordOffset = 0;

    std::vector<std::array<uint8_t, 512>> data;
};