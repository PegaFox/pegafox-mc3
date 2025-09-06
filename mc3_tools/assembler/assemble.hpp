#include <cstdint>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <map>

#include <glm/common.hpp>

#include "../mc3_utils.hpp"

typedef std::pair<std::array<uint8_t, 2>, std::string> asmOperation;
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

uint16_t parseInt(std::map<std::string, std::array<uint16_t, 2>>& variables, const std::string& token, std::string* varName = nullptr, uint16_t* locationSize = nullptr)
{
  if (token[0] >= '0' && token[0] <= '9')
  {
    std::size_t length = 0;
    uint16_t value = std::stoi(token, &length, 0);

    if (length == token.size())
    {
      return value;
    }
  } else
  {
    if (varName != nullptr)
    {
      *varName = token;
    }

    if (!variables.contains(token))
    {
      variables[token][1] = -1;
    } else if (locationSize != nullptr && variables[token][1] != uint16_t(-1))
    {
      *locationSize = variables[token][1];
    }
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

uint8_t getReg3(const std::vector<std::string>& tokens, std::size_t& t, uint8_t firstReg, std::map<std::string, std::array<uint16_t, 2>>& variables, std::string* varName = nullptr)
{
  uint8_t out = getReg(tokens[t]) << 5;

  if (isReg(tokens[t+1]))
  {
    t++;
    out |= getReg(tokens[t]) << 2;
  } else if (tokens[t+1][0] >= '0' && tokens[t+1][0] <= '9')
  {
    t++;
    out |= (parseInt(variables, tokens[t], varName) << 1) | 1;
  } else
  {
    out = (firstReg << 5) | (out >> 3);
  }

  return out;
}

uint8_t getReg2Off6(const std::vector<std::string>& tokens, std::size_t& t, std::map<std::string, std::array<uint16_t, 2>>& variables, std::string* varName = nullptr, uint16_t* locationSize = nullptr)
{
  uint8_t out = getReg(tokens[t]) << 6;

  if (tokens[t+1] == "+" || tokens[t+1] == "-")
  {
    t++;
    out |= tokens[t] == "+" ? parseInt(variables, tokens[++t], varName, locationSize) & 0x3F : -parseInt(variables, tokens[++t], varName, locationSize) & 0x3F;
  }

  return out;
}

asmOperation getALUbinaryOperation(const std::vector<std::string>& tokens, std::size_t& t, std::map<std::string, std::array<uint16_t, 2>>& variables, Opcode regCode, Opcode valCode)
{
  asmOperation operation;

  uint8_t mainReg = getReg(tokens[t++]);

  if (isReg(tokens[t]))
  {
    operation.first[0] = (uint8_t(regCode) << 3) | mainReg;

    operation.first[1] = getReg3(tokens, t, mainReg, variables, &operation.second);
  } else
  {
    operation.first[0] = (uint8_t(valCode) << 3) | mainReg;

    operation.first[1] = parseInt(variables, tokens[t], &operation.second);
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
    operation.first[1] = tokens[t] == "+" ? parseInt(variables, tokens[++t], &operation.second) & 0xFF : -parseInt(variables, tokens[++t], &operation.second) & 0xFF;
  }

  return operation;
}

std::array<asmOperation, 3> getSetImmediateOperation(uint8_t mainReg, uint16_t value, uint8_t minCommandCount = 1)
{
  std::array<asmOperation, 3> operations;
  uint8_t commandCount;

  if ((value & 0xFF) == value)
  {
    commandCount = 1;

    operations[0].first[0] = (uint8_t(Opcode::SetVal) << 3) | mainReg;
    operations[0].first[1] = value;
  } else if ((value & 0xFF00) == value)
  {
    commandCount = 2;

    operations[0].first[0] = (uint8_t(Opcode::SetVal) << 3) | mainReg;
    operations[0].first[1] = value >> 8;

    operations[1].first[0] = (uint8_t(Opcode::LshVal) << 3) | mainReg;
    operations[1].first[1] = 8;
  } else if (value > 0xFF00)
  {
    commandCount = 2;

    operations[0].first[0] = (uint8_t(Opcode::SetVal) << 3) | mainReg;
    operations[0].first[1] = 0;

    operations[1].first[0] = (uint8_t(Opcode::SubVal) << 3) | mainReg;
    operations[1].first[1] = -(value & 0xFF);
  } else
  {
    commandCount = 3;

    operations[0].first[0] = (uint8_t(Opcode::SetVal) << 3) | mainReg;
    operations[0].first[1] = value >> 8;

    operations[1].first[0] = (uint8_t(Opcode::LshVal) << 3) | mainReg;
    operations[1].first[1] = 8;

    operations[2].first[0] = (uint8_t(Opcode::AddVal) << 3) | mainReg;
    operations[2].first[1] = value & 0xFF;
  }

  while (commandCount < minCommandCount)
  {
    operations[commandCount].first[0] = (uint8_t(Opcode::AddReg) << 3) | 0;
    operations[commandCount].first[1] = 0;

    commandCount++;
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
        if (!variables.contains(program[o].second))
        {
          std::cerr << "ERROR: Label/Variable '" << program[o].second << "' not defined\n";
          exit(-4);
        }

        uint8_t operationChainLength = 1;
        if (program[o+1].second == program[o].second)
        {
          operationChainLength++;
          if (program[o+2].second == program[o].second)
          {
            operationChainLength++;
          }
        }

        std::map<std::string, std::array<uint16_t, 2>>::iterator var = variables.find(program[o].second);
        switch (Opcode(program[o].first[0] >> 3))
        {
          case Opcode::OrVal:
          case Opcode::AndVal:
          case Opcode::XorVal:
          case Opcode::LshVal:
          case Opcode::RshVal:
          case Opcode::AddVal:
          case Opcode::SubVal:
          case Opcode::SetVal:
          /*case Opcode::JmpZ:
          case Opcode::JmpNz:
          case Opcode::JmpC:
          case Opcode::JmpNc:
          case Opcode::JmpS:
          case Opcode::JmpNs:
          case Opcode::JmpO:
          case Opcode::JmpNo:*/ {
            std::array<asmOperation, 3> operations = getSetImmediateOperation(program[o].first[0] & 0x7, variables[program[o].second][0], 1 + iterations/200);

            uint8_t operationSize = 1;
            if (operations[1].first[0] != 0x00)
            {
              operationSize++;
              if (operations[2].first[0] != 0x00)
              {
                operationSize++;
              }
            }

            if (operationSize != operationChainLength)
            {
              changed = true;
              if (operationSize > operationChainLength)
              {
                std::move_backward(program.data()+o, program.end()-3, program.end()-(3-(operationSize-operationChainLength)));
                currentBinarySize += (operationSize-operationChainLength)*2;
              } else if (operationSize < operationChainLength)
              {
                std::move(program.data()+o+operationChainLength, program.end(), program.data()+o+operationSize);
                currentBinarySize -= (operationChainLength-operationSize)*2;
              }

              for (std::pair<const std::string, std::array<uint16_t, 2>>& variable: variables)
              {
                if (variable.second[0] > o << 1)
                {
                  variable.second[0] += (operationSize-operationChainLength) << 1;
                }
              }
            }

            if (program[o].first != operations[0].first)
            {
              changed = true;
            }

            program[o].first = operations[0].first;
            program[o].second = var->first;
            if (operationSize > 1)
            {
              if (program[o+1].first != operations[1].first)
              {
                changed = true;
              }
                
              program[o+1].first = operations[1].first;
              program[o+1].second = var->first;
              if (operationSize > 2)
              {
                if (program[o+2].first != operations[2].first)
                {
                  changed = true;
                }

                program[o+2].first = operations[2].first;
                program[o+2].second = var->first;
              }
            }

            o += operationSize-1;
            break;
          } /*case Opcode::OrReg:
          case Opcode::AndReg:
          case Opcode::XorReg:
          case Opcode::LshReg:
          case Opcode::RshReg:
          case Opcode::AddReg:
          case Opcode::SubReg:
          case Opcode::SetReg:*/
            program[o].first[1] = (program[o].first[1]&0b11100000) | (((program[o].first[1]&0b11111) + var->second[0]) & 0b11111);
            break;
          case Opcode::LodB:
          case Opcode::LodW:
          case Opcode::StrB:
          case Opcode::StrW:

            program[o].first[1] = (program[o].first[1]&0b11000000) | (((program[o].first[1]&0b111111) + var->second[0]) & 0b111111);
            break;
          case Opcode::SingleOp:
            std::cerr << "ERROR: Unexpected Label/Variable name '" << program[o].second << "' in instruction requiring no operands\n";
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
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::LshReg, Opcode::LshVal);
      pos += 2;
    } else if (tokens[t] == "rsh")
    {
      program[pos >> 1] = getALUbinaryOperation(tokens, ++t, variables, Opcode::RshReg, Opcode::RshVal);
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
        uint16_t value = parseInt(variables, tokens[t], &program[pos >> 1].second);

        std::array<asmOperation, 3> operations = getSetImmediateOperation(mainReg, value);
        
        if (operations[0].first[0] != 0x00)
        {
          program[pos >> 1].first = operations[0].first;

          if (operations[1].first[0] != 0x00)
          {
            pos += 2;
            program[pos >> 1].first = operations[1].first;

            if (operations[2].first[0] != 0x00)
            {
              pos += 2;
              program[pos >> 1].first = operations[2].first;
            }
          }
        }
      } else
      {
        uint16_t locationSize = 0;

        if (tokens[t].find('@') == std::string::npos)
        {
          locationSize = 1 + (parseInt(variables, tokens[t++]) != 1);
        }

        uint8_t value = getReg2Off6(tokens, ++t, variables, &program[pos >> 1].second, locationSize == 0 ? &locationSize : nullptr);

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
          locationSize = parseInt(variables, tokens[t++]);
        }

        if (locationSize == 1 || locationSize == 2)
        {
          uint8_t value = getReg2Off6(tokens, ++t, variables, &program[pos >> 1].second, &locationSize);

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
          getReg2Off6(tokens, ++t, variables, nullptr, nullptr);

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
      pos = parseInt(variables, tokens[++t]);
    } else if (tokens[t] == "var")
    {
      std::string name = tokens[++t];

      if (tokens[t+1] == "[")
      {
        t++;
        variables[name][1] = parseInt(variables, tokens[++t]);
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
        variables[name][0] = parseInt(variables, tokens[++t]);
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
