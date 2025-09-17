#include <vector>

#include "c-compiler-lib/generate_IR.hpp"

struct VarData
{
  uint16_t index;

  PrimitiveType type;
};

const std::string instructionSegmentReg = "m0";
const std::string dataSegmentReg = "m1";
const std::string stackSegmentReg = "m2";

void loadFromStack(std::vector<std::string>& assembly, uint16_t index, uint16_t size, uint8_t reg)
{
  if (index > 32)
  {
    assembly.insert(assembly.end(), {
      "sub", stackSegmentReg, std::to_string(index-32),
      "set", std::string("r") + char('0'+reg), std::to_string(size), "@", stackSegmentReg, "-", "32",
      "add", stackSegmentReg, std::to_string(index-32),
    });
  } else
  {
    assembly.insert(assembly.end(), {
      "set", std::string("r") + char('0'+reg), std::to_string(size), "@", stackSegmentReg, "-", std::to_string(index)
    });
  }
}

void loadFromMem(std::vector<std::string>& assembly, const std::map<std::string, VarData>& variableMap, const std::string& var, uint8_t reg)
{
  if (var.find("_NullExpression") != std::string::npos)
  {
    assembly.insert(assembly.end(), {
      "set", std::string("r") + char('0'+reg), "1",
    });
  } else if (variableMap.at(var).index != uint16_t(-1))
  {
    loadFromStack(assembly, variableMap.at(var).index, variableMap.at(var).type.size, reg);
  } else
  {
    assembly.insert(assembly.end(), {
      "add", dataSegmentReg, "var_"+var, "-", "static_data",
      "set", std::string("r") + char('0'+reg), std::to_string(variableMap.at(var).type.size), "@", dataSegmentReg,
      "sub", dataSegmentReg, "var_"+var, "-", "static_data",
    });
  }
}

void storeToStack(std::vector<std::string>& assembly, uint16_t index, uint16_t size, uint8_t reg)
{
  if (index > 32)
  {
    assembly.insert(assembly.end(), {
      "sub", stackSegmentReg, std::to_string(index-32),
      "put", std::string("r") + char('0'+reg), std::to_string(size), "@", stackSegmentReg, "-", "32",
      "add", stackSegmentReg, std::to_string(index-32),
    });
  } else
  {
    assembly.insert(assembly.end(), {
      "put", std::string("r") + char('0'+reg), std::to_string(size), "@", stackSegmentReg, "-", std::to_string(index)
    });
  }
}

void storeToMem(std::vector<std::string>& assembly, const std::map<std::string, VarData>& variableMap, const std::string& var, uint8_t reg)
{
  if (variableMap.at(var).index != uint16_t(-1))
  {
    storeToStack(assembly, variableMap.at(var).index, variableMap.at(var).type.size, reg);
  } else
  {
    assembly.insert(assembly.end(), {
      "add", dataSegmentReg, "var_"+var, "-", "static_data",
      "put", std::string("r") + char('0'+reg), std::to_string(variableMap.at(var).type.size), "@", dataSegmentReg,
      "sub", dataSegmentReg, "var_"+var, "-", "static_data",
    });
  }
}


void loadOperand(std::vector<std::string>& assembly, const std::map<std::string, VarData>& variableMap, const std::string& var, uint8_t reg)
{
  if (var.find_first_not_of("0123456789") != std::string::npos)
  {
    loadFromMem(assembly, variableMap, var, reg);
  } else if (!var.empty())
  {
    assembly.insert(assembly.end(), {
      "set", std::string("r") + char('0'+reg), var
    });
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
  std::vector<std::string> argStack;
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

  std::vector<IRprogram::Function> initFunctions;
  IRprogram::Function mainFunction;
  for (std::size_t f = 0; f < IR.program.size(); f++)
  {
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
      switch (op.code)
      {
        case Operation::Set:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*false, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::GetAddress:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*false, */stackTop, op.type};
          }

          if (variableMap[op.operands[1]].index != uint16_t(-1))
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          // lvalue dereference
          loadFromMem(assembly, variableMap, op.operands[0], 3);

          loadOperand(assembly, variableMap, op.operands[1], 4);

          assembly.insert(assembly.end(), {
            "put", "d0", std::to_string(op.type.size), "@", "m3"
          });

          break;
        case Operation::DereferenceRValue:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          // rvalue dereference
          if (op.operands[1].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "set", "d1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 5);
          }

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
          return assembly;
          break;
        case Operation::SetModulo:
          return assembly;
          break;
        case Operation::SetBitwiseAND:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          
          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          
          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);
          assembly.insert(assembly.end(), {
            "set", "d0", "d0",
            "set", "d0", "FLAGS",
          });

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
            "rsh", "d0", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 4);
          break;
        case Operation::SetGreaterOrEqual:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

          assembly.insert(assembly.end(), {
            "set", "d1", "0",
            "sub", "d1", "d0"
          });

          storeToMem(assembly, variableMap, op.operands[0], 5);
          break;
        case Operation::LogicalNOT:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

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
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 4);

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
          loadOperand(assembly, variableMap, op.operands[0], 4);

          assembly.insert(assembly.end(), {
            "put", "d0", std::to_string(op.type.size), "@", stackSegmentReg, "+", "2",
            "set", "d0", "2", "@", stackSegmentReg,
            "jnz", "d0"
          });

          break;
        case Operation::AddArg:
          argStack.emplace_back(op.operands[0]);
          break;
        case Operation::Call: {
          if (!variableMap.contains(op.operands[1]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          assembly.insert(assembly.end(), {
            "sub", stackSegmentReg, std::to_string(stackTop+op.type.size+2),
            "set", "d0", instructionSegmentReg,
            "add", "d0", "lbl_" + std::to_string(uintptr_t(&op)) + "_ReturnAddress", "-", "text",
            "put", "d0", "2", "@", stackSegmentReg,
          });

          uint16_t argSize = 0;
          for (const std::string& arg: argStack)
          {
            loadOperand(assembly, variableMap, arg, 4);

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
