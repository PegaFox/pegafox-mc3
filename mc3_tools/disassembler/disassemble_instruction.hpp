#include <cstdint>
#include <string>

#include "../mc3_utils.hpp"

std::string disassembleInstruction(uint8_t firstByte, uint8_t secondByte)
{
  const std::string asmInstructions[32] = {
    [(int)Opcode::OrVal] = "or ",
    [(int)Opcode::AndVal] = "and ",
    [(int)Opcode::XorVal] = "xor ",
    //[(int)Opcode::LshVal] = "N/A ",
    //[(int)Opcode::RshVal] = "N/A ",
    [(int)Opcode::AddVal] = "add ",
    [(int)Opcode::SubVal] = "sub ",
    [(int)Opcode::SingleOp] = "",
    [(int)Opcode::OrReg] = "or ",
    [(int)Opcode::AndReg] = "and ",
    [(int)Opcode::XorReg] = "xor ",
    [(int)Opcode::LshReg] = "lsh ",
    [(int)Opcode::RshReg] = "rsh ",
    [(int)Opcode::LrotReg] = "lrt ",
    [(int)Opcode::RrotReg] = "rrt ",
    [(int)Opcode::AddReg] = "add ",
    [(int)Opcode::SubReg] = "sub ",
    [(int)Opcode::SetVal] = "set ",
    [(int)Opcode::LodB] = "set ",
    [(int)Opcode::LodW] = "set ",
    [(int)Opcode::StrB] = "put ",
    [(int)Opcode::StrW] = "put ",
    [(int)Opcode::JmpZ] = "jz ",
    [(int)Opcode::JmpNz] = "jnz ",
    [(int)Opcode::JmpC] = "jc ",
    [(int)Opcode::JmpNc] = "jnc ",
    [(int)Opcode::JmpS] = "js ",
    [(int)Opcode::JmpNs] = "jns ",
    [(int)Opcode::JmpO] = "jo ",
    [(int)Opcode::JmpNo] = "jno ",
    [(int)Opcode::OpOnly] = ""
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
    case OperationType::Reg3:
      result += "r" + std::to_string(firstByte & 0x7) + " r" + std::to_string(secondByte >> 5);
      
      if (secondByte & 1)
      {
        result += " " + std::to_string((secondByte >> 1) & 0xF);
      } else
      {
        result += " r" + std::to_string((secondByte >> 2) & 0x7);
      }
      break;
    case OperationType::RegValue:
      result += "r" + std::to_string(firstByte & 0x7) + " " + std::to_string(1 + ((firstByte >> 3) & 1)) + "@m" + std::to_string(secondByte >> 6);
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
