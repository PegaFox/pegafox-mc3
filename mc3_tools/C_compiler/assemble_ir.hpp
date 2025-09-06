#include <vector>

#include "c-compiler-lib/generate_IR.hpp"

struct VarData
{
  uint16_t index;

  PrimitiveType type;
};

void loadFromStack(std::vector<std::string>& assembly, uint16_t index, uint16_t size, uint8_t reg)
{
  if (index > 32)
  {
    assembly.insert(assembly.end(), {
      "sub", "m0", std::to_string(index-32),
      "set", std::string("r") + char('0'+reg), std::to_string(size), "@", "m0", "-", "32",
      "add", "m0", std::to_string(index-32),
    });
  } else
  {
    assembly.insert(assembly.end(), {
      "set", std::string("r") + char('0'+reg), std::to_string(size), "@", "m0", "-", std::to_string(index)
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
      "set", std::string("r") + char('0'+reg), "var_"+var,
      "set", std::string("r") + char('0'+reg), std::to_string(variableMap.at(var).type.size), "@", std::string("r") + char('0'+reg),
    });
  }
}

void storeToStack(std::vector<std::string>& assembly, uint16_t index, uint16_t size, uint8_t reg)
{
  if (index > 32)
  {
    assembly.insert(assembly.end(), {
      "sub", "m0", std::to_string(index-32),
      "put", std::string("r") + char('0'+reg), std::to_string(size), "@", "m0", "-", "32",
      "add", "m0", std::to_string(index-32),
    });
  } else
  {
    assembly.insert(assembly.end(), {
      "put", std::string("r") + char('0'+reg), std::to_string(size), "@", "m0", "-", std::to_string(index)
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
      "set", "m3", "var_"+var,
      "put", std::string("r") + char('0'+reg), std::to_string(variableMap.at(var).type.size), "@", "m3",
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
  std::vector<std::string> assembly = {"pos", "0x0000", "set", "r0", "0xFEFF"};

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

          loadOperand(assembly, variableMap, op.operands[1], 1);

          storeToMem(assembly, variableMap, op.operands[0], 1);
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
              "set", "r1", "m0",
              "sub", "r1", std::to_string(variableMap[op.operands[1]].index)
            });
          } else
          {
            assembly.insert(assembly.end(), {
              "set", "r1", "var_"+op.operands[1],
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::DereferenceLValue:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          // lvalue dereference
          loadFromMem(assembly, variableMap, op.operands[0], 1);

          loadOperand(assembly, variableMap, op.operands[1], 2);

          assembly.insert(assembly.end(), {
            "put", "r2", std::to_string(op.type.size), "@", "r1"
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
              "set", "r1", op.operands[1],
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[1], 1);
          }

          assembly.insert(assembly.end(), {
            "set", "r1", std::to_string(op.type.size), "@", "r1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);

          break;
        case Operation::SetAddition:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "add", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "add", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetSubtraction:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "sub", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetMultiplication:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "set", "r2", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);
          }

          /*
           * r1 = left
           * r2 = right
           * while (r1 != 0)
           * {
           *   if (r1 & 1)
           *   {
           *     result += r2;
           *   }
           *
           *   r1 >> 1;
           *   r2 << 1;
           * }
           */

          assembly.insert(assembly.end(), {
            "set", "r3", "0",
            "set", "d0", "lbl_loop_" + op.operands[0],
            "set", "d1", "lbl_if_" + op.operands[0],
            "lbl_loop_" + op.operands[0] + ":",
            "and", "d2", "r1", "1",
            "jz", "d1",
            "add", "r3", "r2",
            "lbl_if_" + op.operands[0] + ":",
            "lsh", "r2", "1",
            "rsh", "r1", "1",
            "jnz", "d0",
          });

          storeToMem(assembly, variableMap, op.operands[0], 3);

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

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "and", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "and", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetBitwiseOR:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "or", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "or", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetBitwiseXOR:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "xor", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "xor", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetLeftShift:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          
          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "lsh", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "lsh", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetRightShift:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }
          
          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "rsh", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "rsh", "r1", "r2"
            });
          }

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetLogicalAND:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);
          assembly.insert(assembly.end(), {
            "set", "r1", "r1",
            "set", "r1", "FLAGS",
          });

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "set", "r2", std::to_string((std::stoi(op.operands[2]) == 0) << 6),
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "set", "r2", "r2",
              "set", "r2", "FLAGS",
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "or", "r1", "r2",
            "not", "r1",
            "rsh", "r1", "6",
            "and", "r1", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetLogicalOR:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "or", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "or", "r1", "r2"
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "r1", "FLAGS",
            "not", "r1",
            "rsh", "r1", "6",
            "and", "r1", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetEqual:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "sub", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r1", "r2"
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "r1", "FLAGS",
            "rsh", "r1", "6",
            "and", "r1", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetNotEqual:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "sub", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r1", "r2"
            });
          }

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "r1", "FLAGS",
            "not", "r1",
            "rsh", "r1", "6",
            "and", "r1", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetGreater:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "set", "r2", op.operands[2],
              "sub", "r2", "r1"
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r2", "r1"
            });
          }

          assembly.insert(assembly.end(), {
            "rsh", "r2", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 2);
          break;
        case Operation::SetLesser:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "sub", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r1", "r2"
            });
          }

          assembly.insert(assembly.end(), {
            "rsh", "r1", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetGreaterOrEqual:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "sub", "r1", op.operands[2]
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r1", "r2"
            });
          }

          assembly.insert(assembly.end(), {
            "not", "r1",
            "rsh", "r1", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::SetLesserOrEqual:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          if (op.operands[2].find_first_not_of("0123456789") == std::string::npos)
          {
            assembly.insert(assembly.end(), {
              "set", "r2", op.operands[2],
              "sub", "r2", "r1"
            });
          } else
          {
            loadFromMem(assembly, variableMap, op.operands[2], 2);

            assembly.insert(assembly.end(), {
              "sub", "r2", "r1"
            });
          }

          assembly.insert(assembly.end(), {
            "not", "r2",
            "rsh", "r2", "15",
          });

          storeToMem(assembly, variableMap, op.operands[0], 2);
          break;
        case Operation::Negate:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          assembly.insert(assembly.end(), {
            "set", "r2", "0",
            "sub", "r2", "r1"
          });

          storeToMem(assembly, variableMap, op.operands[0], 2);
          break;
        case Operation::LogicalNOT:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "set", "r1", "r1",
            "set", "r1", "FLAGS",
            "rsh", "r1", "6",
            "and", "r1", "1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::BitwiseNOT:
          if (!variableMap.contains(op.operands[0]))
          {
            stackTop += op.type.size;
            variableMap[op.operands[0]] = VarData{/*true, */stackTop, op.type};
          }

          loadOperand(assembly, variableMap, op.operands[1], 1);

          // Get the zero bit from the flags register
          assembly.insert(assembly.end(), {
            "not", "r1",
          });

          storeToMem(assembly, variableMap, op.operands[0], 1);
          break;
        case Operation::Label:
          assembly.insert(assembly.end(), {
            "lbl_" + op.operands[0] + ":"
          });
          break;
        case Operation::Return:
          loadOperand(assembly, variableMap, op.operands[0], 1);

          assembly.insert(assembly.end(), {
            "put", "r1", std::to_string(op.type.size), "@", "m0", "+", "2",
            "set", "r1", "2", "@", "m0",
            "jnz", "r1"
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
            "sub", "m0", std::to_string(op.type.size+2),
            "set", "r1", "lbl_" + std::to_string(uintptr_t(&op)) + "_ReturnAddress",
            "put", "r1", "2", "@", "m0",
          });

          uint16_t argSize = 0;
          for (const std::string& arg: argStack)
          {
            loadOperand(assembly, variableMap, arg, 1);

            argSize += op.type.size;
            assembly.insert(assembly.end(), {
              "put", "r1", "2", "@", "m0", "-", std::to_string(argSize),
            });
          }
          argStack.clear();

          assembly.insert(assembly.end(), {
            "set", "r1", "lbl_" + op.operands[0],
            "jnz", "r1",
            "lbl_" + std::to_string(uintptr_t(&op)) + "_ReturnAddress:",
            "add", "m0", std::to_string(op.type.size+2)
          });

          break;
        } case Operation::Jump:
          assembly.insert(assembly.end(), {
            "set", "r1", "lbl_" + op.operands[0],
            "jnz", "r1"
          });
          break;
        case Operation::JumpIfZero:
          assembly.insert(assembly.end(), {
            "set", "r1", "lbl_" + op.operands[0]
          });

          loadFromMem(assembly, variableMap, op.operands[1], 2);

          assembly.insert(assembly.end(), {
            "set", "r2", "r2",
            "jz", "r1"
          });
          break;
        case Operation::JumpIfNotZero:
          assembly.insert(assembly.end(), {
            "set", "r1", "lbl_" + op.operands[0]
          });

          loadFromMem(assembly, variableMap, op.operands[1], 2);
          assembly.insert(assembly.end(), {
            "set", "r2", "r2",
            "jnz", "r1"
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
