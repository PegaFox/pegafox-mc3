#include <cmath>
#include <iostream>
#include <vector>

#include "c-compiler-lib/generate_IR.hpp"

struct AssembleIrError {};

struct VarData
{
  uint16_t index;

  PrimitiveType type;

  inline bool onStack() const 
  {
    return index != (uint16_t)-1;
  }
};

const constexpr std::string_view intConstant = ".0123456789";

inline bool isImmediate(const std::string& var)
{
  return var.find_first_not_of(intConstant) == std::string::npos;
}

// For instructions that have a result value, this is the operand index of the name of the result value. Otherwise, this is -1
const constexpr int8_t returnIndices[32] = {
  [Operation::Set] = 0,
  [Operation::GetAddress] = 0,
  [Operation::DereferenceLValue] = 0,
  [Operation::DereferenceRValue] = 0,
  [Operation::SetAddition] = 0,
  [Operation::SetSubtraction] = 0,
  [Operation::SetMultiplication] = 0,
  [Operation::SetDivision] = 0,
  [Operation::SetModulo] = 0,
  [Operation::SetLeftShift] = 0,
  [Operation::SetRightShift] = 0,
  [Operation::SetBitwiseAND] = 0,
  [Operation::SetBitwiseOR] = 0,
  [Operation::SetBitwiseXOR] = 0,
  [Operation::SetLogicalAND] = 0,
  [Operation::SetLogicalOR] = 0,
  [Operation::SetEqual] = 0,
  [Operation::SetNotEqual] = 0,
  [Operation::SetGreater] = 0,
  [Operation::SetLesser] = 0,
  [Operation::SetGreaterOrEqual] = 0,
  [Operation::SetLesserOrEqual] = 0,
  [Operation::Negate] = 0,
  [Operation::LogicalNOT] = 0,
  [Operation::BitwiseNOT] = 0,
  [Operation::Label] = -1,
  [Operation::Return] = -1,
  [Operation::AddArg] = -1,
  [Operation::Call] = 1,
  [Operation::Jump] = -1,
  [Operation::JumpIfZero] = -1,
  [Operation::JumpIfNotZero] = -1,
};

const std::string instructionSegmentReg = "m0";
const std::string dataSegmentReg = "m1";
const std::string stackSegmentReg = "m2";

// The largest stack byte offset we can fetch memory from without needing to perform additional operations
const constexpr uint16_t maxFastStackOffset = 32;
void loadFromStack(std::vector<std::string>& assembly, uint16_t index, uint16_t size, uint8_t startReg)
{
  /*if (size > 2)
  {
    std::cerr << "Error: Cannot handle " << size*CHAR_BIT << "-bit values\n";
    throw AssembleIrError();
  }*/

  if (index > maxFastStackOffset)
  {
    assembly.insert(assembly.end(), {
      "sub", stackSegmentReg, std::to_string(index-maxFastStackOffset),
    });

    for (uint8_t w = 0; w < std::ceil(size/2.0f); w++)
    {
      // Amount of memory to move for the current loop iteration.
      std::string chunkSize = size % 2 == 1 && w == size/2 ? "1" : "2";

      assembly.insert(assembly.end(), {
        "set", std::string("r") + char('0'+startReg+w), chunkSize, "@", stackSegmentReg, "-", std::to_string(maxFastStackOffset + w*2),
      });
    }

    assembly.insert(assembly.end(), {
      "add", stackSegmentReg, std::to_string(index-maxFastStackOffset),
    });
  } else
  {
    for (uint8_t w = 0; w < std::ceil(size/2.0f); w++)
    {
      // Amount of memory to move for the current loop iteration.
      std::string chunkSize = size % 2 == 1 && w == size/2 ? "1" : "2";

      assembly.insert(assembly.end(), {
        "set", std::string("r") + char('0'+startReg+w), chunkSize, "@", stackSegmentReg, "-", std::to_string(index + w*2)
      });
    }
  }
}

void loadFromMem(std::vector<std::string>& assembly, const std::map<std::string, VarData>& variableMap, const std::string& var, uint8_t startReg)
{
  if (var.find("_NullExpression") != std::string::npos)
  {
    assembly.insert(assembly.end(), {
      "set", std::string("r") + char('0'+startReg), "1",
    });

    return;
  }

  const VarData& data = variableMap.at(var);

  if (data.onStack())
  {
    loadFromStack(assembly, data.index, data.type.size, startReg);
  } else
  {
    /*if (data.type.size > 2)
    {
      std::cerr << "Error: Cannot handle " << data.type.size*CHAR_BIT << "-bit values\n";
      throw AssembleIrError();
    }*/

    assembly.insert(assembly.end(), {
      "add", dataSegmentReg, "var_"+var, "-", "static_data",
    });

    for (uint8_t w = 0; w < std::ceil(data.type.size/2.0f); w++)
    {
      // Amount of memory to move for the current loop iteration.
      std::string chunkSize = data.type.size % 2 == 1 && w == data.type.size/2 ? "1" : "2";

      assembly.insert(assembly.end(), {
        "set", std::string("r") + char('0'+startReg), chunkSize, "@", dataSegmentReg, "+", std::to_string(w*2),
      });
    }

    assembly.insert(assembly.end(), {
      "sub", dataSegmentReg, "var_"+var, "-", "static_data",
    });
  }
}

void storeToStack(std::vector<std::string>& assembly, uint16_t index, uint16_t size, uint8_t startReg)
{
  /*if (size > 2)
  {
    std::cerr << "Error: Cannot handle " << size*CHAR_BIT << "-bit values\n";
    throw AssembleIrError();
  }*/

  if (index > maxFastStackOffset)
  {
    assembly.insert(assembly.end(), {
      "sub", stackSegmentReg, std::to_string(index-maxFastStackOffset),
    });

    for (uint8_t w = 0; w < std::ceil(size/2.0f); w++)
    {
      // Amount of memory to move for the current loop iteration.
      std::string chunkSize = size % 2 == 1 && w == size/2 ? "1" : "2";

      assembly.insert(assembly.end(), {
        "put", std::string("r") + char('0'+startReg), chunkSize, "@", stackSegmentReg, "-", std::to_string(maxFastStackOffset + w*2),
      });
    }

    assembly.insert(assembly.end(), {
      "add", stackSegmentReg, std::to_string(index-maxFastStackOffset),
    });
  } else
  {
    for (uint8_t w = 0; w < std::ceil(size/2.0f); w++)
    {
      // Amount of memory to move for the current loop iteration.
      std::string chunkSize = size % 2 == 1 && w == size/2 ? "1" : "2";

      assembly.insert(assembly.end(), {
        "put", std::string("r") + char('0'+startReg), chunkSize, "@", stackSegmentReg, "-", std::to_string(index + w*2)
      });
    }
  }
}

void storeToMem(std::vector<std::string>& assembly, const std::map<std::string, VarData>& variableMap, const std::string& var, uint8_t startReg)
{
  const VarData& data = variableMap.at(var);

  if (data.onStack())
  {
    storeToStack(assembly, data.index, data.type.size, startReg);
  } else
  {
    /*if (variableMap.at(var).type.size > 2)
    {
      std::cerr << "Error: Cannot handle " << variableMap.at(var).type.size*CHAR_BIT << "-bit values\n";
      throw AssembleIrError();
    }*/

    assembly.insert(assembly.end(), {
      "add", dataSegmentReg, "var_"+var, "-", "static_data",
    });

    for (uint8_t w = 0; w < std::ceil(data.type.size/2.0f); w++)
    {
      // Amount of memory to move for the current loop iteration.
      std::string chunkSize = data.type.size % 2 == 1 && w == data.type.size/2 ? "1" : "2";

      assembly.insert(assembly.end(), {
        "put", std::string("r") + char('0'+startReg), chunkSize, "@", dataSegmentReg, "+", std::to_string(w*2),
      });
    }

    assembly.insert(assembly.end(), {
      "sub", dataSegmentReg, "var_"+var, "-", "static_data",
    });
  }
}

void loadOperand(std::vector<std::string>& assembly, const std::map<std::string, VarData>& variableMap, PrimitiveType dataType, const std::string& var, uint8_t startReg)
{
  if (!isImmediate(var))
  {
    loadFromMem(assembly, variableMap, var, startReg);
  } else if (!var.empty())
  {
    std::array<uint8_t, 16> varValue = Constant::parseValue({}, dataType, var);
    for (uint8_t w = 0; w < std::ceil(dataType.size/2.0f); w++)
    {
      assembly.insert(assembly.end(), {
        "set", std::string("r") + char('0'+startReg+w), std::to_string(*(uint16_t*)varValue.begin())
      });
    }
  }
}

std::vector<std::string> assembleIR(IRprogram& IR)
{
  std::vector<std::string> assembly = {
    "pos", "0",
    "set", instructionSegmentReg, "text",
    "set", dataSegmentReg, "static_data",
    "set", stackSegmentReg, "65279", // sets stack base pointer to 0xFEFF
    "text:",
  };

  std::map<std::string, VarData> variableMap;
  std::vector<std::pair<PrimitiveType, std::string>> argStack;
  uint16_t stackTop;

  std::map<std::size_t, std::string> staticVarsByIdx;
  for (const std::pair<std::string, std::size_t>& var: IR.staticVariables)
  {
    staticVarsByIdx[var.second] = var.first;
  }

  for (std::map<std::size_t, std::string>::const_iterator var = staticVarsByIdx.cbegin(); var != staticVarsByIdx.cend(); var++)
  {
    VarData varData;
    varData.index = -1;
    varData.type.alignment = 1;
    
    std::map<std::size_t, std::string>::const_iterator next = ++var;
    var--;

    if (next == staticVarsByIdx.cend())
    {
      varData.type.size = IR.staticData.size() - var->first;
    } else
    {
      varData.type.size = next->first - var->first;
    }

    variableMap[var->second] = varData;
  }

  // Confirm order of functions in binary
  std::vector<IRprogram::Function> initFunctions;
  IRprogram::Function mainFunction;
  for (std::size_t f = 0; f < IR.program.size(); f++)
  {
    if (IR.program[f].body.empty())
    {
      continue;
    }

    if (IR.program[f].body[0].operands[0] == "main")
    {
      mainFunction = IR.program[f];

      IR.program.erase(IR.program.begin()+f);

      f--;
    } else if (IR.program[f].body[0].code != Operation::Opcode::Label)
    {
      initFunctions.emplace_back(IR.program[f]);

      IR.program.erase(IR.program.begin()+f);

      f--;
    }
  }

  IR.program.insert(IR.program.begin(), mainFunction);
  IR.program.insert(IR.program.begin(), initFunctions.begin(), initFunctions.end());
  
  for (const IRprogram::Function& function: IR.program)
  {
    stackTop = 0;
    for (const std::pair<std::string, PrimitiveType>& parameter: function.parameters)
    {
      stackTop += parameter.second.size;
      variableMap[parameter.first] = VarData{/*false, */stackTop, parameter.second};
    }

    for (const Operation& op: function.body)
    {
      assembly.emplace_back();

      /*// Add return value to variable map
      if (returnIndices[op.code] > -1 && returnIndices[op.code] < 3)
      {
        uint8_t returnIndex = returnIndices[op.code];
        const std::string& returnName = op.operands[returnIndex];

        if (!variableMap.contains(returnName))
        {
          stackTop += op.type.size;
          variableMap[returnName] = VarData{stackTop, op.type};
        }
      }*/

      // Add values to variable map
      for (const std::string operand: op.operands)
      {
        if (!operand.empty() && !isImmediate(operand) && !variableMap.contains(operand))
        {
          stackTop += op.type.size;
          variableMap[operand] = VarData{/*false, */stackTop, op.type};
        }
      }

      switch (op.code)
      {
        case Operation::Set:
          //if (op.type.size <= 2)
          //{
            loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

            storeToMem(assembly, variableMap, op.operands[0], 4);
          //} else if (op.type.size <= 4)
          //{
            /*
             * Example assembly
             *
             *   Loading immediate:
             *     set d0 0xDEAD
             *     set d1 0xBEEF
             *
             *     put d0 2@m0-1
             *     put d1 2@m0
             *
             *   Loading from memory:
             *     set d0 2@m0-3
             *     set d1 2@m0-2
             *
             *     put d0 2@m0-1
             *     put d1 2@m0
             */
            /*loadOperand(assembly, variableMap, op.operands[1], 4);
            loadOperand(assembly, variableMap, op.operands[1], 5);

            storeToMem(assembly, variableMap, op.operands[0], 4);
            storeToMem(assembly, variableMap, op.operands[0], 5);
          }*/
          break;
        case Operation::GetAddress:
          if (variableMap[op.operands[1]].onStack())
          {
            assembly.insert(assembly.end(), {
              "set", "d0", stackSegmentReg,
              "sub", "d0", std::to_string(variableMap[op.operands[1]].index)
            });
          } else
          {
            assembly.insert(assembly.end(), {
              "set", "d0", dataSegmentReg,
              "add", "d0", "var_"+op.operands[1], "-", "static_data",
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::DereferenceLValue:
          // lvalue dereference
          loadFromMem(assembly, variableMap, op.operands[0], 3);

          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          assembly.insert(assembly.end(), {
            "put", "d0", std::to_string(op.type.size), "@", "m3"
          });

          break;
        case Operation::DereferenceRValue:
          // rvalue dereference
          if (isImmediate(op.operands[1]))
          {
            assembly.insert(assembly.end(), {
              "set", "m3", op.operands[1],
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[1], 3);
          }

          assembly.insert(assembly.end(), {
            "set", "d0", std::to_string(op.type.size), "@", "m3",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);

          break;
        case Operation::SetAddition:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "add", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "add", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetSubtraction:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "sub", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetMultiplication:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          loadOperand(assembly, variableMap, op.type, op.operands[2], 5);

          /*
           * d0 = left
           * d1 = right
           * d2 = result
           * while (d0 != 0)
           * {
           *   if (d0 & 1)
           *   {
           *     d2 += d1;
           *   }
           *
           *   d1 << 1;
           *   d0 >> 1;
           * }
           */

          assembly.insert(assembly.end(), {
            "set", "d2", "0",
            "set", "m3", instructionSegmentReg,
            "add", "m3", "lbl_loop_" + op.operands[0], "-", "text",
            "lbl_loop_" + op.operands[0] + ":",
            "add", "m3", "lbl_if_" + op.operands[0], "-", "lbl_loop_" + op.operands[0],
            "and", "d3", "d0", "1",
            "jz", "m3", 
            "add", "d2", "d1",
            "lbl_if_" + op.operands[0] + ":",
            "lsh", "d1", "1",
            "sub", "m3", "lbl_if_" + op.operands[0], "-", "lbl_loop_" + op.operands[0],
            "rsh", "d0", "1",
            "jnz", "m3",
          });

          storeToMem(assembly, variableMap, op.operands[0], 6);

          break;
        case Operation::SetDivision:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          loadOperand(assembly, variableMap, op.type, op.operands[2], 5);

          /*
           * d0 = left
           * d1 = right
           * d2 = result
           * while (d0 >= 0)
           * {
           *   d0 -= d1;
           *   d2 += 1;
           * }
           * d2 -= 1;
           */

          assembly.insert(assembly.end(), {
            "set", "d2", "0",
            "set", "m3", instructionSegmentReg,
            "add", "m3", "lbl_loop_" + op.operands[0], "-", "text",
            "lbl_loop_" + op.operands[0] + ":",
            "sub", "d0", "d1",
            "add", "d2", "1",
            "jns", "m3",
            "sub", "d2", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 6);

          break;
        case Operation::SetModulo:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          loadOperand(assembly, variableMap, op.type, op.operands[2], 6);

          /*
           * d0 = left
           * d2 = right
           * while (d0 >= 0)
           * {
           *   d0 -= d2;
           * }
           * d0 += d2;
           */

          assembly.insert(assembly.end(), {
            "set", "m3", instructionSegmentReg,
            "add", "m3", "lbl_loop_" + op.operands[0], "-", "text",
            "lbl_loop_" + op.operands[0] + ":",
            "sub", "d0", "d2",
            "jns", "m3",
            "add", "d0", "d2",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);

          break;
        case Operation::SetBitwiseAND:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "and", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "and", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetBitwiseOR:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "or", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "or", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetBitwiseXOR:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "xor", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "xor", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetLeftShift:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "lsh", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "lsh", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetRightShift:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "rsh", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "rsh", "d0", "d1"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetLogicalAND:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);
          assembly.insert(assembly.end(), {
            "set", "d0", "d0",
            "set", "d0", "FLAGS",
          });

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "set", "d1", std::to_string((std::stoi(op.operands[2]) == 0) << 6),
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "set", "d1", "d1",
              "set", "d1", "FLAGS",
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "or", "d0", "d1",
            "not", "d0",
            "rsh", "d0", "6",
            "and", "d0", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetLogicalOR:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "or", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "or", "d0", "d1"
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "d0", "FLAGS",
            "not", "d0",
            "rsh", "d0", "6",
            "and", "d0", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetEqual:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "sub", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d0", "d1"
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "d0", "FLAGS",
            "rsh", "d0", "6",
            "and", "d0", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetNotEqual:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "sub", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d0", "d1"
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "d0", "FLAGS",
            "not", "d0",
            "rsh", "d0", "6",
            "and", "d0", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetGreater:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "set", "d1", op.operands[2],
              "sub", "d1", "d0"
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d1", "d0"
            });
          }

          assembly.insert(assembly.end(), {
            "rsh", "d1", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 5);
          break;
        case Operation::SetLesser:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "sub", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d0", "d1"
            });
          };

          assembly.insert(assembly.end(), {
            "rsh", "d0", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetGreaterOrEqual:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "sub", "d0", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d0", "d1"
            });
          }

          assembly.insert(assembly.end(), {
            "not", "d0",
            "rsh", "d0", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetLesserOrEqual:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          if (isImmediate(op.operands[2]))
          {
            assembly.insert(assembly.end(), {
              "set", "d1", op.operands[2],
              "sub", "d1", "d0"
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);

            assembly.insert(assembly.end(), {
              "sub", "d1", "d0"
            });
          }

          assembly.insert(assembly.end(), {
            "not", "d1",
            "rsh", "d1", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::Negate:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          assembly.insert(assembly.end(), {
            "set", "d1", "0",
            "sub", "d1", "d0"
          });

          storeToMem(assembly, variableMap, op.operands[0], 5);
          break;
        case Operation::LogicalNOT:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "d0", "d0",
            "set", "d0", "FLAGS",
            "rsh", "d0", "6",
            "and", "d0", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::BitwiseNOT:
          loadOperand(assembly, variableMap, op.type, op.operands[1], 4);

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "not", "d0",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::Label:
          assembly.insert(assembly.end(), {
            "lbl_" + op.operands[0] + ":"
          });
          break;
        case Operation::Return:
          loadOperand(assembly, variableMap, op.type, op.operands[0], 4);

          assembly.insert(assembly.end(), {
            "put", "d0", std::to_string(op.type.size), "@", stackSegmentReg, "+", "2",
            "set", "d0", "2", "@", stackSegmentReg,
            "jnz", "d0"
          });

          break;
        case Operation::AddArg:
          argStack.emplace_back(op.type, op.operands[0]);
          break;
        case Operation::Call: {
          if (!variableMap.contains(op.operands[1]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[1]] = VarData{/*true, */stackTop, op.type};
          }

          assembly.insert(assembly.end(), {
            "sub", stackSegmentReg, std::to_string(stackTop+op.type.size+2),
            "set", "d0", instructionSegmentReg,
            "add", "d0", "lbl_" + std::to_string(uintptr_t(&op)) + "_ReturnAddress", "-", "text",
            "put", "d0", "2", "@", stackSegmentReg,
          });

          uint16_t argSize = 0;
          for (const std::pair<PrimitiveType, std::string>& arg: argStack)
          {
            loadOperand(assembly, variableMap, arg.first, arg.second, 4);

            argSize += op.type.size;
            assembly.insert(assembly.end(), {
              "put", "d0", "2", "@", stackSegmentReg, "-", std::to_string(argSize),
            });
          }
          argStack.clear();

          assembly.insert(assembly.end(), {
            "set", "d0", instructionSegmentReg,
            "add", "d0", "lbl_" + op.operands[0], "-", "text",
            "jnz", "d0",
            "lbl_" + std::to_string(uintptr_t(&op)) + "_ReturnAddress:",
            "add", stackSegmentReg, std::to_string(stackTop+op.type.size+2)
          });

          break;
        } case Operation::Jump:
          assembly.insert(assembly.end(), {
            "set", "d0", instructionSegmentReg,
            "add", "d0", "lbl_" + op.operands[0], "-", "text",
            "jnz", "d0"
          });
          break;
        case Operation::JumpIfZero:
          loadFromMem(assembly, variableMap, op.operands[1], 4);

          assembly.insert(assembly.end(), {
            "set", "d1", instructionSegmentReg,
            "add", "d1", "lbl_" + op.operands[0], "-", "text",
            "set", "d0", "d0",
            "jz", "d1"
          });
          break;
        case Operation::JumpIfNotZero:
          loadFromMem(assembly, variableMap, op.operands[1], 4);

          assembly.insert(assembly.end(), {
            "set", "d1", instructionSegmentReg,
            "add", "d1", "lbl_" + op.operands[0], "-", "text",
            "set", "d0", "d0",
            "jnz", "d1"
          });
          break;
      }
    }
  }

  assembly.emplace_back("static_data:");

  for (uint16_t b = 0; b < IR.staticData.size(); b++)
  {
    if (!staticVarsByIdx.empty() && staticVarsByIdx.cbegin()->first == b)
    {
      assembly.insert(assembly.end(), {
        "var", "var_"+staticVarsByIdx.cbegin()->second, "[", std::to_string(variableMap[staticVarsByIdx.cbegin()->second].type.size), "]", "=",
      });

      staticVarsByIdx.erase(staticVarsByIdx.cbegin());
    }

    assembly.emplace_back(std::to_string(IR.staticData[b]));
  }

  return assembly;
}
