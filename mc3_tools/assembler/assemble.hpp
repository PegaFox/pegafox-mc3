#include <cstdint>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <map>

#include <glm/common.hpp>

#include "../mc3_utils.hpp"

#include "expression-parser/expression.hpp"

// using namespace rather than scoped enum allows for implicit integral conversions
namespace OperatorType
{
  enum OperatorType: uint16_t
  {
    Constant,
    Identifier,
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    None = uint16_t(-1),
  };
}

const Operator::StaticData operatorTypes[OperatorType::Max] = {
  [OperatorType::Constant] = {.pattern = std::regex("[0-9]+(\\.[0-9]+)?"), .hasChildren = false},
  [OperatorType::Identifier] = {.pattern = std::regex("[a-zA-Z_]\\w*\\b"), .hasChildren = false},
  [OperatorType::Add] = {.pattern = std::regex("\\+"), .hasChildren = true, .priority = 1, .leftPriority = false},
  [OperatorType::Subtract] = {.pattern = std::regex("\\-"), .hasChildren = true, .priority = 1, .leftPriority = false},
  [OperatorType::Multiply] = {.pattern = std::regex("\\*"), .hasChildren = true, .priority = 2, .leftPriority = false},
  [OperatorType::Divide] = {.pattern = std::regex("/"), .hasChildren = true, .priority = 2, .leftPriority = false}
};

typedef std::pair<std::array<uint8_t, 2>, std::vector<Operator>> asmOperation;
typedef std::array<asmOperation, INT16_MAX+1> asmProgram;

bool isReg(const std::string& reg)
{
  if (reg.size() == 2)
  {
    if (reg[0] == 'm' || reg[0] == 'd')
    {
      return reg[1] >= '0' && reg[1] <= '3';
    } else if (reg[0] == 'r')
    {
      return reg[1] >= '0' && reg[1] <= '7';
    }
  }

  return false;
}

uint8_t getReg(const std::string& reg)
{
  if (!isReg(reg))
  {
    std::cerr << "ERROR: Invalid register name: " << reg << '\n';
    exit(-2);
  }

  if (reg[0] == 'm' || reg[0] == 'r')
  {
    return reg[1] - '0';
  } else
  {
    return reg[1] - '0' + 4;
  }
}

uint16_t parseConstantExpression(const std::map<std::string, std::array<uint16_t, 2>>& variables, const std::vector<Operator>& expression, bool* trivial = nullptr, uint8_t currentIndex = 0)
{
  if (expression[currentIndex].type == OperatorType::Identifier)
  {
    if (trivial)
    {
      *trivial = false;
    }

    if (variables.contains(std::string(expression[currentIndex].token)))
    {
      return variables.at(std::string(expression[currentIndex].token))[0];
    } else
    {
      return 0;
    }
  } else if (expression[currentIndex].type == OperatorType::Constant)
  {
    return std::stoi(std::string(expression[currentIndex].token));
  }

  const Operator& parent = expression[currentIndex];
  const Operator& leftChild = expression[parent.leftIndex];
  const Operator& rightChild = expression[parent.rightIndex];

  uint16_t leftValue = 0;
  uint16_t rightValue = 0;

  if (operatorTypes[leftChild.type].hasChildren)
  {
    bool leftTrivial = true;
    leftValue = parseConstantExpression(variables, expression, &leftTrivial);

    if (trivial)
    {
      *trivial &= leftTrivial;
    }
  } else if (leftChild.type == OperatorType::Identifier)
  {
    if (trivial)
    {
      *trivial = false;
    }

    if (variables.contains(std::string(leftChild.token)))
    {
      leftValue = variables.at(std::string(leftChild.token))[0];
    }
  } else
  {
    leftValue = std::stoi(std::string(leftChild.token));
  }

  if (operatorTypes[rightChild.type].hasChildren)
  {
    bool rightTrivial = true;
    rightValue = parseConstantExpression(variables, expression, &rightTrivial);

    if (trivial)
    {
      *trivial &= rightTrivial;
    }
  } else if (rightChild.type == OperatorType::Identifier)
  {
    if (trivial)
    {
      *trivial = false;
    }

    if (variables.contains(std::string(rightChild.token)))
    {
      rightValue = variables.at(std::string(rightChild.token))[0];
    }
  } else
  {
    rightValue = std::stoi(std::string(rightChild.token));
  }

  switch (parent.type)
  {
    case OperatorType::Add:
      return leftValue + rightValue;
    case OperatorType::Subtract:
      return leftValue - rightValue;
    case OperatorType::Multiply:
      return leftValue * rightValue;
    case OperatorType::Divide:
      return leftValue / rightValue;
    default:
      if (trivial)
      {
        *trivial = false;
      }
      return 0;
  }
}

// After execution, t is the index of the last token consumed
uint16_t parseInt(std::map<std::string, std::array<uint16_t, 2>>& variables, const std::vector<std::string>& tokens, std::size_t& t, std::vector<Operator>* expression = nullptr)
{
  std::size_t expressionEnd = t;
  bool lastHadChildren = true;
  for (; expressionEnd <= tokens.size(); expressionEnd++)
  {
    if (expressionEnd == tokens.size())
    {
      break;
    }

    OperatorType::OperatorType tokenType = OperatorType::OperatorType(Operator::parseType(operatorTypes, OperatorType::Max, tokens[expressionEnd]));

    // Break if two identifiers/constants are adjacent
    if (tokenType == OperatorType::None || operatorTypes[tokenType].hasChildren == lastHadChildren)
    {
      break;
    } else
    {
      lastHadChildren = !lastHadChildren;
    }
  }

  std::vector<Operator> expressionTree = Operator::parseExpression(operatorTypes, OperatorType::None, tokens.begin()+t, tokens.begin()+expressionEnd);
  t = expressionEnd-1;

  //std::cout << "\n" << expressionTree[0].type << "\n";

  bool trivial = true;
  uint16_t value = parseConstantExpression(variables, expressionTree, &trivial);

  if (trivial)
  {
    return value;
  } else
  {
    if (expression)
    {
      *expression = expressionTree;
    }

    /*if (!variables.contains(token))
    {
      variables[token][1] = -1;
    }*/
  }
  return 0;
}

// After execution, t is the index of the last token consumed
std::vector<uint8_t> parseArray(const std::vector<std::string>& tokens, std::size_t& t)
{
  std::vector<uint8_t> result;

  while (t < tokens.size() && tokens[t][0] >= '0' && tokens[t][0] <= '9')
  {
    uint64_t value = std::stoll(tokens[t], nullptr, 0);

    for (uint8_t o = 0; o < 64; o += 8)
    {
      if (uint8_t(value >> o))
      {
        result.emplace_back(uint8_t(value >> o));
      }
    }

    t++;
  }
  t--;

  return result;
}

uint8_t getReg3(const std::vector<std::string>& tokens, std::size_t& t, uint8_t firstReg, std::map<std::string, std::array<uint16_t, 2>>& variables, std::vector<Operator>* expression = nullptr)
{
  uint8_t out = getReg(tokens[t]) << 5;

  if (isReg(tokens[t+1]))
  {
    t++;
    out |= getReg(tokens[t]) << 2;
  } else if (tokens[t+1][0] >= '0' && tokens[t+1][0] <= '9')
  {
    t++;
    out |= (parseInt(variables, tokens, t, expression) << 1) | 1;
  } else
  {
    out = (firstReg << 5) | (out >> 3);
  }

  return out;
}

uint8_t getReg2Off6(const std::vector<std::string>& tokens, std::size_t& t, std::map<std::string, std::array<uint16_t, 2>>& variables, std::vector<Operator>* expression = nullptr)
{
  uint8_t out = getReg(tokens[t]) << 6;

  if (tokens[t+1] == "+" || tokens[t+1] == "-")
  {
    t++;
    out |= tokens[t] == "+" ? parseInt(variables, tokens, ++t, expression) & 0x3F : -parseInt(variables, tokens, ++t, expression) & 0x3F;
  }

  return out;
}

asmOperation getALUbinaryOperation(const std::vector<std::string>& tokens, std::size_t& t, std::map<std::string, std::array<uint16_t, 2>>& variables, Opcode regCode, Opcode valCode)
{
  asmOperation operation;

  uint8_t mainReg = getReg(tokens[t++]);

  if (valCode == Opcode::None || isReg(tokens[t]))
  {
    if (!isReg(tokens[t]))
    {
      t--;
    }

    operation.first[0] = (uint8_t(regCode) << 3) | mainReg;

    operation.first[1] = getReg3(tokens, t, mainReg, variables, &operation.second);
  } else
  {
    operation.first[0] = (uint8_t(valCode) << 3) | mainReg;

    operation.first[1] = parseInt(variables, tokens, t, &operation.second);
  }

  return operation;
}

std::array<uint8_t, 2> getUnaryOperation(const std::vector<std::string>& tokens, std::size_t& t, Opcode opcode)
{
  std::array<uint8_t, 2> operation;

  uint8_t mainReg = getReg(tokens[t]);

  operation[0] = (uint8_t(opcode) << 3) | mainReg;
  operation[1] = 0;

  return operation;
}

asmOperation getJumpOperation(const std::vector<std::string>& tokens, std::size_t& t, std::map<std::string, std::array<uint16_t, 2>>& variables, Opcode opcode)
{
  asmOperation operation;

  uint8_t mainReg = getReg(tokens[t]);

  operation.first[0] = (uint8_t(opcode) << 3) | mainReg;

  if (t < tokens.size()-1 && (tokens[t+1] == "+" || tokens[t+1] == "-"))
  {
    t++;
    operation.first[1] = tokens[t] == "+" ? parseInt(variables, tokens, ++t, &operation.second) & 0xFF : -parseInt(variables, tokens, ++t, &operation.second) & 0xFF;
  }

  return operation;
}

std::vector<asmOperation> getImm8Operation(Opcode opcode, uint8_t mainReg, uint16_t value, uint8_t minCommandCount = 0)
{
  std::vector<asmOperation> operations;
  operations.reserve(minCommandCount);

  if ((value & 0xFF) == value)
  {
    operations.emplace_back(std::array<uint8_t, 2>{uint8_t((uint8_t(opcode) << 3) | mainReg), (uint8_t)value}, std::vector<Operator>{});
  } else
  {
    switch (opcode)
    {
      case Opcode::OrVal:
      case Opcode::AndVal:
      case Opcode::XorVal:
        if ((value & 0xFF00) == value)
        {
          operations.insert(operations.end(), {
            {{uint8_t((uint8_t(Opcode::LrotReg) << 3) | mainReg), uint8_t((mainReg << 5) | (8 << 1) | 1)}, {}},
            {{uint8_t((uint8_t(opcode) << 3) | mainReg), uint8_t(value >> 8)}, {}},
            {{uint8_t((uint8_t(Opcode::LrotReg) << 3) | mainReg), uint8_t((mainReg << 5) | (8 << 1) | 1)}, {}},
          });
        } else
        {
          operations.insert(operations.end(), {
            {{uint8_t((uint8_t(Opcode::LrotReg) << 3) | mainReg), uint8_t((mainReg << 5) | (8 << 1) | 1)}, {}},
            {{uint8_t((uint8_t(opcode) << 3) | mainReg), uint8_t(value >> 8)}, {}},
            {{uint8_t((uint8_t(Opcode::LrotReg) << 3) | mainReg), uint8_t((mainReg << 5) | (8 << 1) | 1)}, {}},
            {{uint8_t((uint8_t(opcode) << 3) | mainReg), uint8_t(value & 0xFF)}, {}},
          });
        }
        break;
      /*case Opcode::LshVal:
      case Opcode::RshVal:*/
        std::cerr << "ERROR: attempt to resolve unimplemented multiop instruction (opcode = " << int(opcode) << ", value = " << value << ")\n";
        break;
      case Opcode::AddVal:
        while (value > 0xFF)
        {
          operations.emplace_back(std::array<uint8_t, 2>{uint8_t((uint8_t(Opcode::AddVal) << 3) | mainReg), 0xFF}, std::vector<Operator>{});

          value -= 0xFF;
        }

        if (value > 0)
        {
          operations.emplace_back(std::array<uint8_t, 2>{uint8_t((uint8_t(Opcode::AddVal) << 3) | mainReg), uint8_t(value)}, std::vector<Operator>{});
        }
        break;
      case Opcode::SubVal:
      case Opcode::JmpZ:
      case Opcode::JmpNz:
      case Opcode::JmpC:
      case Opcode::JmpNc:
      case Opcode::JmpS:
      case Opcode::JmpNs:
      case Opcode::JmpO:
      case Opcode::JmpNo:
        std::cerr << "ERROR: attempt to resolve unimplemented multiop instruction (opcode = " << int(opcode) << ", value = " << value << ")\n";
        break;
      case Opcode::SetVal:
        if ((value & 0xFF00) == value)
        {
          operations.insert(operations.end(), {
            {{uint8_t((uint8_t(Opcode::SetVal) << 3) | mainReg), uint8_t(value >> 8)}, {}},
            {{uint8_t((uint8_t(Opcode::LshReg) << 3) | mainReg), uint8_t((mainReg << 5) | (8 << 1) | 1)}, {}},
          });
        } else if (value > 0xFF00)
        {
          operations.insert(operations.end(), {
            {{uint8_t((uint8_t(Opcode::SetVal) << 3) | mainReg), 0}, {}},
            {{uint8_t((uint8_t(Opcode::SubVal) << 3) | mainReg), uint8_t(-(value & 0xFF))}, {}},
          });
        } else
        {
          operations.insert(operations.end(), {
            {{uint8_t((uint8_t(Opcode::SetVal) << 3) | mainReg), uint8_t(value >> 8)}, {}},
            {{uint8_t((uint8_t(Opcode::LshReg) << 3) | mainReg), uint8_t((mainReg << 5) | (8 << 1) | 1)}, {}},
            {{uint8_t((uint8_t(Opcode::AddVal) << 3) | mainReg), uint8_t(value & 0xFF)}, {}},
          });
        }
        break;
    }
  }

  // If space is needed, pad with no-ops
  while (operations.size() < minCommandCount)
  {
    operations.emplace_back(std::array<uint8_t, 2>{1, 0}, std::vector<Operator>{});
  }

  return operations;
}

asmProgram resolveLabels(asmProgram program, std::map<std::string, std::array<uint16_t, 2>>& variables, uint16_t& currentBinarySize)
{
  uint16_t iterations = 0;
  bool changed = true;
  while (changed)
  {
    changed = false;

    for (uint16_t o = 0; o < program.size(); o++)
    {
      if (!program[o].second.empty())
      {
        std::vector<Operator> expression = program[o].second;
        uint16_t value = parseConstantExpression(variables, expression);

        // I may want to reimplement this functionality elsewhere
        /*if (!variables.contains(program[o].second))
        {
          std::cerr << "ERROR: Label/Variable '" << program[o].second << "' not defined\n";
          exit(-4);
        }*/

        uint8_t operationChainLength = 1;
        for (; program[o+operationChainLength].second == expression; operationChainLength++) {}
        /*if (program[o+1].second == expression)
        {
          operationChainLength++;
          if (program[o+2].second == expression)
          {
            operationChainLength++;
          }
        }*/

        switch (Opcode(program[o].first[0] >> 3))
        {
          case Opcode::OrVal:
          case Opcode::AndVal:
          case Opcode::XorVal:
          //case Opcode::LshVal:
          //case Opcode::RshVal:
          case Opcode::AddVal:
          case Opcode::SubVal:
          case Opcode::SetVal:
          case Opcode::JmpZ:
          case Opcode::JmpNz:
          case Opcode::JmpC:
          case Opcode::JmpNc:
          case Opcode::JmpS:
          case Opcode::JmpNs:
          case Opcode::JmpO:
          case Opcode::JmpNo: {
            std::vector<asmOperation> operations = getImm8Operation(Opcode(program[o].first[0] >> 3), program[o].first[0] & 0x7, value, 0 + iterations/200);

            /*if (operations[1].first[0] != 0x00)
            {
              operationSize++;
              if (operations[2].first[0] != 0x00)
              {
                operationSize++;
              }
            }*/

            /*std::cout << "chain len: " << (int)operationChainLength << "\n";
            std::cout << "iterations: " << (int)iterations << "\n";
            std::cout << "operation index: " << (int)o << "\n";
            std::cout << "expression len: " << (int)program[o].second.size() << "\n";*/
            if (operations.size() != operationChainLength)
            {
              changed = true;
              if (operations.size() > operationChainLength)
              {
                //std::move_backward(program.data()+o, program.end()-3, program.end()-(3-(operations.size()-operationChainLength)));
                std::move_backward(program.data()+o, program.end()-(operations.size()-operationChainLength), program.end());
                currentBinarySize += (operations.size()-operationChainLength)*2;
              } else if (operations.size() < operationChainLength)
              {
                std::move(program.data()+o+operationChainLength, program.end(), program.data()+o+operations.size());
                currentBinarySize -= (operationChainLength-operations.size())*2;
              }

              // Update new variable positions
              for (std::pair<const std::string, std::array<uint16_t, 2>>& variable: variables)
              {
                if (variable.second[0] > o << 1)
                {
                  variable.second[0] += (operations.size()-operationChainLength) << 1;
                }
              }
            }

            for (uint8_t i = 0; i < operations.size(); i++)
            {
              if (program[o+i].first != operations[i].first)
              {
                changed = true;
              }
                  
              program[o+i].first = operations[i].first;
              program[o+i].second = expression;
            }

            o += operations.size()-1;
            break;
          } case Opcode::OrReg:
          case Opcode::AndReg:
          case Opcode::XorReg:
          case Opcode::LshReg:
          case Opcode::RshReg:
          case Opcode::LrotReg:
          case Opcode::RrotReg:
          case Opcode::AddReg:
          case Opcode::SubReg:
            std::cerr << "ERROR: attempt to resolve non-trivial expression for unimplemented imm4 instruction (opcode = " << int(program[o].first[0] >> 3) << ")\n";
            //program[o].first[1] = (program[o].first[1]&0b11100000) | (((program[o].first[1]&0b11111) + var->second[0]) & 0b11111);
            break;
          case Opcode::LodB:
          case Opcode::LodW:
          case Opcode::StrB:
          case Opcode::StrW:
            std::cerr << "ERROR: attempt to resolve non-trivial expression for unimplemented imm6 instruction (opcode = " << int(program[o].first[0] >> 3) << ")\n";
            //program[o].first[1] = (program[o].first[1]&0b11000000) | (((program[o].first[1]&0b111111) + var->second[0]) & 0b111111);
            break;
          case Opcode::SingleOp:
            std::cerr << "ERROR: Unexpected Expression in instruction requiring no operands\n";
            exit(-5);
            break;
        }
      }
    }

    iterations++;
  }

  return program;
}

std::vector<uint8_t> assemble(const std::vector<std::string>& tokens)
{
  asmProgram program;

  std::map<std::string, std::array<uint16_t, 2>> variables;

  std::vector<uint8_t> binary;

  std::size_t pos = 0;
  for (std::size_t t = 0; t < tokens.size(); t++)
  {
    if (tokens[t] == "or")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::OrReg, Opcode::OrVal);
      pos += 2;
    } else if (tokens[t] == "and")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::AndReg, Opcode::AndVal);
      pos += 2;
    } else if (tokens[t] == "xor")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::XorReg, Opcode::XorVal);
      pos += 2;
    } else if (tokens[t] == "not")
    {
      program[pos >> 1].first = getUnaryOperation(tokens, ++t, Opcode::SingleOp);
      pos += 2;
    } else if (tokens[t] == "lsh")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::LshReg, Opcode::None);
      pos += 2;
    } else if (tokens[t] == "rsh")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::RshReg, Opcode::None);
      pos += 2;
    } else if (tokens[t] == "add")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::AddReg, Opcode::AddVal);
      pos += 2;
    } else if (tokens[t] == "sub")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::SubReg, Opcode::SubVal);
      pos += 2;
    } else if (tokens[t] == "set")
    {// SetReg, SetVal, LodB, LodW, GetF
      t++;
      uint8_t mainReg = getReg(tokens[t++]);

      if (isReg(tokens[t]))
      {
        program[pos >> 1].first[0] = (uint8_t(Opcode::AddReg) << 3) | mainReg;
        program[pos >> 1].first[1] = (getReg(tokens[t]) << 5) | 1;
        //program[pos >> 1].first[0] = (uint8_t(Opcode::SetReg) << 3) | mainReg;
        //program[pos >> 1].first[1] = getReg3(tokens, t, mainReg, variables, &program[pos >> 1].second);
      } else if (tokens[t] == "FLAGS")
      {
        program[pos >> 1].first[0] = (uint8_t(Opcode::SingleOp) << 3) | mainReg;

        program[pos >> 1].first[1] = (uint8_t)SingleOpcode::GetF;
      } else if (tokens[t].find('@') == std::string::npos && (tokens.size() == t+1 || tokens[t+1].find('@') == std::string::npos))
      {
        uint16_t value = parseInt(variables, tokens, t, &program[pos >> 1].second);

        std::vector<asmOperation> operations = getImm8Operation(Opcode::SetVal, mainReg, value);
        
        for (uint8_t i = 0; i < operations.size(); i++)
        {
          program[pos >> 1].first = operations[i].first;
          pos += 2;
        }
        pos -= 2;
      } else
      {
        uint16_t locationSize = 0;

        if (tokens[t].find('@') == std::string::npos)
        {
          locationSize = 1 + (parseInt(variables, tokens, t) != 1);
          t++;
        }

        uint8_t value = getReg2Off6(tokens, ++t, variables, &program[pos >> 1].second);

        if (locationSize == 1)
        {
          program[pos >> 1].first[0] = (uint8_t(Opcode::LodB) << 3) | mainReg;
        } else
        {
          program[pos >> 1].first[0] = (uint8_t(Opcode::LodW) << 3) | mainReg;
        }

        program[pos >> 1].first[1] = value;
      }
      pos += 2;
    } else if (tokens[t] == "put")
    {// PutI, StrB, StrW
      t++;
      uint8_t mainReg = getReg(tokens[t++]);

      if (tokens[t] == "IVEC")
      {
        program[pos >> 1].first[0] = (uint8_t(Opcode::SingleOp) << 3) | mainReg;
        program[pos >> 1].first[1] = (uint8_t)SingleOpcode::PutI;
      } else
      {
        uint16_t locationSize = 0;

        if (tokens[t].find('@') == std::string::npos)
        {
          locationSize = parseInt(variables, tokens, t);
          t++;
        }

        if (locationSize == 1 || locationSize == 2)
        {
          uint8_t value = getReg2Off6(tokens, ++t, variables, &program[pos >> 1].second);

          if (locationSize == 1)
          {
            program[pos >> 1].first[0] = (uint8_t(Opcode::StrB) << 3) | mainReg;
          } else
          {
            program[pos >> 1].first[0] = (uint8_t(Opcode::StrW) << 3) | mainReg;
          }

          program[pos >> 1].first[1] = value;
        } else
        {
          getReg2Off6(tokens, ++t, variables);

          pos -= 2;
        }
      }
      
      pos += 2;
    } else if (tokens[t] == "jz")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpZ);
      pos += 2;
    } else if (tokens[t] == "jnz")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpNz);
      pos += 2;
    } else if (tokens[t] == "jc")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpC);
      pos += 2;
    } else if (tokens[t] == "jnc")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpNc);
      pos += 2;
    } else if (tokens[t] == "js")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpS);
      pos += 2;
    } else if (tokens[t] == "jns")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpNs);
      pos += 2;
    } else if (tokens[t] == "jo")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpO);
      pos += 2;
    } else if (tokens[t] == "jno")
    {
      program[pos >> 1] = getJumpOperation(tokens, ++t, variables, Opcode::JmpNo);
      pos += 2;
    } else if (tokens[t] == "iret")
    {
      program[pos >> 1].first[0] = (uint8_t(Opcode::OpOnly) << 3) | ((uint16_t)OnlyOpcode::IRet >> 8);
      program[pos >> 1].first[1] = (uint8_t)OnlyOpcode::IRet;
      pos += 2;
    } else if (tokens[t] == "inc")
    {
      uint8_t mainReg = getReg(tokens[++t]);

      program[pos >> 1].first[0] = (uint8_t(Opcode::AddVal) << 3) | mainReg;
      program[pos >> 1].first[1] = 1;
      pos += 2;
    } else if (tokens[t] == "dec")
    {
      uint8_t mainReg = getReg(tokens[++t]);

      program[pos >> 1].first[0] = (uint8_t(Opcode::SubVal) << 3) | mainReg;
      program[pos >> 1].first[1] = 1;
      pos += 2;
    } else if (tokens[t] == "exit")
    {
      program[pos >> 1].first[0] = (uint8_t(Opcode::SetVal) << 3) | 0;
      program[pos >> 1].first[1] = 0;
      pos += 2;

      program[pos >> 1].first[0] = (uint8_t(Opcode::JmpZ) << 3) | 0;
      program[pos >> 1].first[1] = int8_t(-2);
      pos += 2;
    } else if (tokens[t] == "pos")
    {
      pos = parseInt(variables, tokens, ++t);
    } else if (tokens[t] == "var")
    {
      std::string name = tokens[++t];

      if (tokens[t+1] == "[")
      {
        t++;
        variables[name][1] = parseInt(variables, tokens, ++t);
        if (tokens[++t] != "]")
        {
          std::cerr << "ERROR: Expected ']' after variable size specification\n";
          exit(-3);
        }
      } else
      {
        variables[name][1] = -1;
      }

      if (tokens[t+1] == "@")
      {
        t++;
        variables[name][0] = parseInt(variables, tokens, ++t);
        binary.reserve(variables[name][0]+variables[name][1]);
      } else
      {
        variables[name][0] = pos;
      }

      if (tokens[t+1] == "=")
      {
        t++;
        std::vector<uint8_t> data = parseArray(tokens, ++t);

        if (variables[name][1] == uint16_t(-1))
        {
          variables[name][1] = data.size();
        }

        for (std::size_t i = 0; i < glm::min(variables[name][1], uint16_t(data.size())); i++)
        {
          program[(variables[name][0]+i) >> 1].first[(variables[name][0]+i) & 1] = data[i];
        }
      }

      if (variables[name][0] == pos && variables[name][1] != uint16_t(-1))
      {
        pos += variables[name][1];
      }
    } else if (tokens[t].back() == ':')
    {
      variables[tokens[t].substr(0, tokens[t].size()-1)][0] = pos;
    } else
    {
      std::cerr << "ERROR: Unrecognized token '" << tokens[t] << "'\n";
    }

    binary.reserve(pos);
  }

  uint16_t binarySize = binary.capacity();
  program = resolveLabels(program, variables, binarySize);

  binary.resize(std::ceil(float(binarySize)/2.0f) * 2.0f);
  for (std::size_t i = 0; i < binary.size(); i += 2)
  {
    binary[i] = program[i >> 1].first[0];
    binary[i+1] = program[i >> 1].first[1];
  }

  return binary;
}
