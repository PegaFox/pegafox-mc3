#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <regex>

struct Operator
{
  enum Type
  {
    Constant,
    Identifier,
    Add,
    Subtract,
    Multiply,
    Divide,
    None
  } type = None;

  struct StaticData
  {
    std::regex pattern = std::regex("^");
    bool hasChildren = false;

    uint8_t priority = 0;
    bool leftPriority = false;
  };
  
  static const StaticData operatorData[Type::None];

  std::string_view token;
  uint8_t leftIndex = -1;
  uint8_t rightIndex = -1;

  bool operator==(const Operator& rhs) const
  {
    return token == rhs.token && leftIndex == rhs.leftIndex && rightIndex == rhs.rightIndex;
  }

  bool operator!=(const Operator& rhs) const
  {
    return !(*this == rhs);
  }

  static Type parseType(const std::string& token)
  {
    for (Type t = Type(0); t < Type::None; t = Type(t + 1))
    {
      if (std::regex_match(token, operatorData[t].pattern))
      {
        return t;
      }
    }

    return None;
  }

  static bool isChild(Operator op)
  {
    return operatorData[op.type].hasChildren == false || (op.leftIndex != uint8_t(-1) && op.rightIndex != uint8_t(-1));
  }

  static uint8_t findNextParent(const std::vector<Operator>& storage, uint8_t childIndex)
  {
    for (uint8_t o = childIndex+1; o < storage.size(); o++)
    {
      if (!isChild(storage[o]))
      {
        return o;
      }
    }

    return -1;
  }

  static uint8_t findLastParent(const std::vector<Operator>& storage, uint8_t childIndex)
  {
    for (int16_t o = (int16_t)childIndex-1; o >= 0; o--)
    {
      if (!isChild(storage[o]))
      {
        return o;
      }
    }

    return -1;
  }

  template <typename InputIterator>
  static std::vector<Operator> parseExpression(InputIterator begin, InputIterator end)
  {
    std::vector<Operator> storage;

    for (InputIterator str = begin; str != end; str++)
    {
      storage.push_back(Operator{parseType(*str), *str, uint8_t(-1), uint8_t(-1)});
    }

    uint8_t rootIndex = 0;
    bool changed = true;
    while (changed)
    {
      changed = false;
      for (uint8_t o = 0; o < storage.size(); o++)
      {
        if (isChild(storage[o]) && (operatorData[storage[o].type].hasChildren || (storage[o].leftIndex == uint8_t(-1) && storage[o].rightIndex == uint8_t(-1))))
        {
          uint8_t leftParentIndex = findLastParent(storage, o);
          uint8_t rightParentIndex = findNextParent(storage, o);

          bool leftAssociative = (leftParentIndex != uint8_t(-1) && operatorData[storage[leftParentIndex].type].leftPriority) || (rightParentIndex != uint8_t(-1) && operatorData[storage[rightParentIndex].type].leftPriority);
          uint8_t leftPriority = leftParentIndex != uint8_t(-1) ? operatorData[storage[leftParentIndex].type].priority : 0;
          uint8_t rightPriority = rightParentIndex != uint8_t(-1) ? operatorData[storage[rightParentIndex].type].priority : 0;

          if (leftPriority == 0 && rightPriority == 0)
          {
            continue;
          }

          if (leftPriority == rightPriority)
          {
            leftPriority += !leftAssociative;
            rightPriority += leftAssociative;
          }

          if (leftPriority > rightPriority)
          {
            storage[leftParentIndex].rightIndex = o;

            if (!operatorData[storage[o].type].hasChildren)
            {
              storage[o].leftIndex = 0;
            }

            if (rootIndex == o)
            {
              rootIndex = leftParentIndex;
            }

            changed = true;
          } else
          {
            storage[rightParentIndex].leftIndex = o;

            if (!operatorData[storage[o].type].hasChildren)
            {
              storage[o].rightIndex = 0;
            }

            if (rootIndex == o)
            {
              rootIndex = rightParentIndex;
            }

            changed = true;
          }
        }
      }
    }

    storage.insert(storage.begin(), storage[rootIndex]);
    storage.erase(storage.begin() + rootIndex+1);
    for (uint8_t o = 0; o < storage.size(); o++)
    {
      if (operatorData[storage[o].type].hasChildren)
      {
        if (storage[o].leftIndex == rootIndex)
        {
          storage[o].leftIndex = 0;
        } if (storage[o].leftIndex < rootIndex)
        {
          storage[o].leftIndex++;
        }

        if (storage[o].rightIndex == rootIndex)
        {
          storage[o].rightIndex = 0;
        } if (storage[o].rightIndex < rootIndex)
        {
          storage[o].rightIndex++;
        }
      }
    }

    return storage;
  }
};

const Operator::StaticData Operator::operatorData[Operator::Type::None] = {
  [Constant] = {.pattern = std::regex("[0-9]+(\\.[0-9]+)?"), .hasChildren = false},
  [Identifier] = {.pattern = std::regex("[a-zA-Z_]\\w*\\b"), .hasChildren = false},
  [Add] = {.pattern = std::regex("\\+"), .hasChildren = true, .priority = 1, .leftPriority = false},
  [Subtract] = {.pattern = std::regex("\\-"), .hasChildren = true, .priority = 1, .leftPriority = false},
  [Multiply] = {.pattern = std::regex("\\*"), .hasChildren = true, .priority = 2, .leftPriority = false},
  [Divide] = {.pattern = std::regex("/"), .hasChildren = true, .priority = 2, .leftPriority = false}
};
