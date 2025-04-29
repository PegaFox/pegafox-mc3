#include <vector>

#include "c-compiler-lib/generate_IR.hpp"

std::vector<std::string> assembleIR(const std::vector<Operation>& IR)
{
  std::vector<std::string> assembly = {"pos", "0x0000", "set", "r0", "0xFEFF"};
  
  struct VarPos
  {
    bool isRegist;
    uint16_t index;
  };

  std::map<std::string, VarPos> variableMap;
  
  uint16_t stackTop = 0;

  for (const Operation& op: IR)
  {
    switch (op.code)
    {
      case Operation::Set:
        if (!variableMap.contains(op.operands[0]))
        {
          variableMap[op.operands[0]] = VarPos{false, stackTop++};
        }

        if (op.operands[1].find_first_not_of("0123456789") == std::string::npos)
        {
          assembly.insert(assembly.end(), {
            "set", "r1", op.operands[1]
          });
        } else
        {
          assembly.insert(assembly.end(), {
            "set", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[1]].index)
          });
        }

        assembly.insert(assembly.end(), {
          "put", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[0]].index)
        });
        break;
      case Operation::GetAddress:

        break;
      case Operation::Dereference:

        break;
      case Operation::SetAddition:
        if (!variableMap.contains(op.operands[0]))
        {
          variableMap[op.operands[0]] = VarPos{true, stackTop++};
        }

        if (op.operands[1].find_first_not_of("0123456789") == std::string::npos)
        {
          assembly.insert(assembly.end(), {
            "set", "r1", op.operands[1]
          });
        } else
        {
          assembly.insert(assembly.end(), {
            "set", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[1]].index)
          });
        }

        if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
        {
          assembly.insert(assembly.end(), {
            "add", "r1", op.operands[2]
          });
        } else
        {
          assembly.insert(assembly.end(), {
            "set", "r2", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[2]].index),
            "add", "r1", "r2"
          });
        }

        assembly.insert(assembly.end(), {
          "put", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[0]].index)
        });
        break;
      case Operation::SetSubtraction:

        break;
      case Operation::SetMultiplication:

        break;
      case Operation::SetDivision:

        break;
      case Operation::SetModulo:

        break;
      case Operation::SetBitwiseAND:

        break;
      case Operation::SetBitwiseOR:

        break;
      case Operation::SetBitwiseXOR:

        break;
      case Operation::SetLeftShift:
        if (!variableMap.contains(op.operands[0]))
        {
          variableMap[op.operands[0]] = VarPos{true, stackTop++};
        }

        if (op.operands[1].find_first_not_of("0123456789") == std::string::npos)
        {
          assembly.insert(assembly.end(), {
            "set", "r1", op.operands[1]
          });
        } else
        {
          assembly.insert(assembly.end(), {
            "set", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[1]].index)
          });
        }

        if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
        {
          assembly.insert(assembly.end(), {
            "lsh", "r1", op.operands[2]
          });
        } else
        {
          assembly.insert(assembly.end(), {
            "set", "r2", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[2]].index),
            "lsh", "r1", "r2"
          });
        }

        assembly.insert(assembly.end(), {
          "put", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[0]].index)
        });
        break;
      case Operation::SetRightShift:

        break;
      case Operation::SetLogicalAND:

        break;
      case Operation::SetLogicalOR:

        break;
      case Operation::SetEqual:

        break;
      case Operation::SetNotEqual:

        break;
      case Operation::SetGreater:

        break;
      case Operation::SetLesser:

        break;
      case Operation::SetGreaterOrEqual:

        break;
      case Operation::SetLesserOrEqual:

        break;
      case Operation::Negate:

        break;
      case Operation::LogicalNOT:

        break;
      case Operation::BitwiseNOT:

        break;
      case Operation::Label:

        break;
      case Operation::Return:
        if (op.operands[0].find_first_not_of("0123456789") != std::string::npos)
        {
          assembly.insert(assembly.end(), {
            "set", "r1", "1", "@", "m0", "-", std::to_string(variableMap[op.operands[0]].index),
          });
        } else if (!op.operands[0].empty())
        {
          assembly.insert(assembly.end(), {
            "set", "r1", op.operands[0]
          });
        }

        assembly.insert(assembly.end(), {
          "put", "r1", "1", "@", "m0", "+", "2",
          "set", "r1", "2", "@", "m0",
          "jnz", "r1"
        });

        break;
      case Operation::AddArg:

        break;
      case Operation::Call:

        break;
      case Operation::Jump:

        break;
      case Operation::JumpIfZero:

        break;
      case Operation::JumpIfNotZero:

        break;
    }
  }

  return assembly;
}
