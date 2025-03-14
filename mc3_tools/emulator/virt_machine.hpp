#include "../mc3_utils.hpp"

class VirtMachine
{
  public:
    Bus<uint16_t> bus;
    uint16_t regs[8] = {0};
    uint16_t pc = 0;
    uint16_t intVec = 0;

    struct Flags
    {
      bool carry: 1;
      bool overflow: 1;
      bool zero: 1;
      bool sign: 1;
      uint8_t bitsSet: 4;
    } flags = {0, 0, 0, 0, 0};

    struct InterruptStateBackup
    {
      uint16_t regs[8];
      uint16_t pc;
      Flags flags;
    } intState;

    bool inInterrupt = false;

    class IntQueue
    {
      public:
        bool push(uint16_t value)
        {
          if (begin != end)
          {
            data[end++] = value;
            end %= data.size();
            return true;
          } else
          {
            return false;
          }
        }

        uint16_t pop()
        {
          if (!empty())
          {
            uint16_t value = data[begin++];
            begin %= data.size();
            return value;
          } else
          {
            return 0;
          }
        }

        bool empty() const
        {
          return (begin+1)%data.size() == end;
        }
      private:
        std::array<uint16_t, 16> data;
        uint8_t begin = 0;
        uint8_t end = 0;
    } intQueue;

    void hardwareInterrupt(uint16_t interruptID)
    {
      if (intVec == 0)
      {
        return;
      }

      intQueue.push(interruptID);
    }

    void tickClock()
    {
      handleInstruction();

      if (intVec != 0 && !inInterrupt && !intQueue.empty())
      {
        handleInterrupt();
      }
    }

  private:
    void handleInstruction()
    {
      uint8_t first = bus.read(pc++);
      uint8_t second = bus.read(pc++);

      switch (Opcode(first >> 3))
      {
        case Opcode::OrVal:
          regs[first & 0x07] |= second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::AndVal:
          regs[first & 0x07] &= second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::XorVal:
          regs[first & 0x07] ^= second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::SingleOp:
          switch (SingleOpcode(second))
          {
            case SingleOpcode::Not:
              regs[first & 0x07] = ~regs[first & 0x07];
              updateFlags(regs[first & 0x07]);
              break;
            case SingleOpcode::GetF:
              regs[first & 0x07] = (flags.carry << 7) | (flags.overflow << 6) | (flags.zero << 5) | (flags.sign << 4) | flags.bitsSet;
              updateFlags(regs[first & 0x07]);
              break;
            case SingleOpcode::PutI:
              intVec = regs[first & 0x07];
              break;
          }
          break;
        case Opcode::LshVal:
          flags.carry = regs[first & 0x07] >> (16-second);

          regs[first & 0x07] <<= second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::RshVal:
          flags.carry = regs[first & 0x07] << (16-second);

          regs[first & 0x07] >>= second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::AddVal:
          flags.carry = uint16_t(regs[first & 0x07] + second) < second;
          flags.overflow = (regs[first & 0x07] >> 15) == (second >> 15) && (regs[first & 0x07] + second) >> 15 != (regs[first & 0x07] >> 15);

          regs[first & 0x07] += second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::SubVal:
          flags.carry = regs[first & 0x07] - second > second;
          flags.overflow = (regs[first & 0x07] >> 15) != (second >> 15) && (regs[first & 0x07] + second) >> 15 != (regs[first & 0x07] >> 15);

          regs[first & 0x07] -= second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::OrReg:
          regs[first & 0x07] |= regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::AndReg:
          regs[first & 0x07] &= regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::XorReg:
          regs[first & 0x07] ^= regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::LshReg:
          flags.carry = regs[first & 0x07] >> (16-(regs[second >> 5] + int8_t(second & 0x1F)));

          regs[first & 0x07] <<= regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::RshReg:
          flags.carry = regs[first & 0x07] << (16-(regs[second >> 5] + int8_t(second & 0x1F)));

          regs[first & 0x07] >>= regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::AddReg:
          flags.carry = regs[first & 0x07] + (regs[second >> 5] + int8_t(second & 0x1F)) < (regs[second >> 5] + int8_t(second & 0x1F));
          flags.overflow = (regs[first & 0x07] >> 15) == ((regs[second >> 5] + int8_t(second & 0x1F)) >> 15) && (regs[first & 0x07] + (regs[second >> 5] + int8_t(second & 0x1F))) >> 15 != (regs[first & 0x07] >> 15);

          regs[first & 0x07] += regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::SubReg:
          flags.carry = regs[first & 0x07] - (regs[second >> 5] + int8_t(second & 0x1F)) > (regs[second >> 5] + int8_t(second & 0x1F));
          flags.overflow = (regs[first & 0x07] >> 15) != ((regs[second >> 5] + int8_t(second & 0x1F)) >> 15) && (regs[first & 0x07] + (regs[second >> 5] + int8_t(second & 0x1F))) >> 15 != (regs[first & 0x07] >> 15);
          
          regs[first & 0x07] -= regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::SetReg:
          regs[first & 0x07] = regs[second >> 5] + int8_t(second & 0x1F);
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::SetVal:
          regs[first & 0x07] = second;
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::LodB:
          regs[first & 0x07] = bus.read(regs[second >> 6] + int8_t(second & 0x3F));
          updateFlags(regs[first & 0x07]);
          break;
        case Opcode::LodW: {
          uint16_t address = regs[second >> 6] + int8_t(second & 0x3F);

          regs[first & 0x07] = (uint16_t)bus.read(address) | ((uint16_t)bus.read(address + 1) << 8);
          updateFlags(regs[first & 0x07]);
          break;
        } case Opcode::StrB:
          bus.write(regs[second >> 6] + int8_t(second & 0x3F), regs[first & 0x07]);
          break;
        case Opcode::StrW: {
          uint16_t address = regs[second >> 6] + int8_t(second & 0x3F);

          bus.write(address, regs[first & 0x07] & 0xFF);
          bus.write(address + 1, regs[first & 0x07] >> 8);
          break;
        } case Opcode::JmpZ:
          if (flags.zero)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpNz:
          if (!flags.zero)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpC:
          if (flags.carry)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpNc:
          if (!flags.carry)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpS:
          if (flags.sign)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpNs:
          if (!flags.sign)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpO:
          if (flags.overflow)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::JmpNo:
          if (!flags.overflow)
          {
            pc = regs[first & 0x07] + int8_t(second);
          }
          break;
        case Opcode::OpOnly:
          switch (OnlyOpcode((((uint16_t)first << 13) >> 5) | (uint16_t)second))
          {
            case OnlyOpcode::IRet:
              inInterrupt = false;

              std::copy(intState.regs, intState.regs + 8, regs);
              pc = intState.pc;
              flags = intState.flags;
              break;
          }
          break;
      }
    }

    void updateFlags(uint16_t value)
    {
      flags.zero = value == 0;
      flags.sign = value >> 15;

      flags.bitsSet = 0;
      for (int i = 0; i < 16; i++)
      {
        if (value & (1 << i))
        {
          flags.bitsSet++;
        }
      }
    }

    void handleInterrupt()
    {
      inInterrupt = true;

      std::copy(regs, regs + 8, intState.regs);
      intState.pc = pc;
      intState.flags = flags;

      pc = intVec;
      regs[0] = intQueue.pop();
    }
};