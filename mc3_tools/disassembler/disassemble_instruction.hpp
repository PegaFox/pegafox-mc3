#include <string>

std::string disassembleInstruction(uint8_t firstByte, uint8_t secondByte)
{
  const std::string asmInstructions[32] = {
    "or ", //OrVal
    "and ", //AndVal
    "xor ", //XorVal
    "lsh ", //LshVal
    "rsh ", //RshVal
    "add ", //AddVal
    "sub ", //SubVal
    "", //SingleOp
    "or ", //OrReg
    "and ", //AndReg
    "xor ", //XorReg
    "lsh ", //LshReg
    "rsh ", //RshReg
    "add ", //AddReg
    "sub ", //SubReg
    "set ", //SetReg
    "set ", //SetVal
    "set ", //LodB
    "set ", //LodW
    "put ", //StrB
    "put ", //StrW
    "jz ", //JmpZ
    "jnz ", //JmpNz
    "jc ", //JmpC
    "jnc ", //JmpNc
    "js ", //JmpS
    "jns ", //JmpNs
    "jo ", //JmpO
    "jno ", //JmpNo
    "" //OpOnly
  };

  std::string result;

  result = asmInstructions[firstByte >> 3];

  if (Opcode(firstByte >> 3) == Opcode::SingleOp)
  {
    switch (SingleOpcode(secondByte)) {
      case SingleOpcode::Not:
        result += "not ";
        break;
      case SingleOpcode::GetF:
        result += "set ";
        break;
      case SingleOpcode::PutI:
        result += "put ";
        break;
    }
  } else if (Opcode(firstByte >> 3) == Opcode::OpOnly)
  {
    result += "iret";
  }

  switch (opcodeTypes[firstByte >> 3])
  {
    case OperationType::NoOperands:
      break;
    case OperationType::OneOperand:
      switch (SingleOpcode(secondByte)) {
        case SingleOpcode::Not:
          result += "r" + std::to_string(firstByte & 0x7);
          break;
        case SingleOpcode::GetF:
          result += "r" + std::to_string(firstByte & 0x7) + " FLAGS";
          break;
        case SingleOpcode::PutI:
          result += "r" + std::to_string(firstByte & 0x7) + " IVEC";
          break;
      }
      break;
    case OperationType::ValueOperand:
      if (Opcode(firstByte >> 3) >= Opcode::JmpZ)
      {
        result += "r" + std::to_string(firstByte & 0x7);
        if (secondByte)
        {
          if (secondByte & 0x80)
          {
            result += std::to_string(int8_t(secondByte));
          } else
          {
            result += "+" + std::to_string(secondByte);
          }
        }
      } else
      {
        result += "r" + std::to_string(firstByte & 0x7) + " " + std::to_string(secondByte);
      }
      break;
    case OperationType::RegValue:
      result += "r" + std::to_string(firstByte & 0x7) + " r" + std::to_string(secondByte >> 5);
      if (secondByte & 0x1F)
      {
        if (secondByte & 0x10)
        {
          result += std::to_string(int8_t(secondByte << 3) >> 3);
        } else
        {
          result += "+" + std::to_string(secondByte & 0x1F);
        }
      }
      break;
    case OperationType::RegValueExt:
      result += "r" + std::to_string(firstByte & 0x7) + " @m" + std::to_string(secondByte >> 6);
      if (secondByte & 0x3F)
      {
        if (secondByte & 0x20)
        {
          result += std::to_string(int8_t(secondByte << 2) >> 2);
        } else
        {
          result += "+" + std::to_string(secondByte & 0x3F);
        }
      }
      break;
  }

  return result;
}