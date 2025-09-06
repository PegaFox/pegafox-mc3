enum class Opcode
{
  None = -1,
  OrVal,
  AndVal,
  XorVal,
  LshVal,
  RshVal,
  AddVal,
  SubVal,
  SingleOp,
  OrReg,
  AndReg,
  XorReg,
  LshReg,
  RshReg,
  AddReg,
  SubReg,
  SetVal,
  LodB,
  LodW,
  StrB,
  StrW,
  JmpZ,
  JmpNz,
  JmpC,
  JmpNc,
  JmpS,
  JmpNs,
  JmpO,
  JmpNo,
  OpOnly
};

enum class SingleOpcode
{
  None = -1,
  Not,
  GetF,
  PutI
};

enum class OnlyOpcode
{
  None = -1,
  IRet
};

enum class OperationType
{
  NoOperands,
  OneOperand,
  ValueOperand,
  Reg3,
  RegValue
};

const OperationType opcodeTypes[32] = {
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::OneOperand,
  OperationType::Reg3,
  OperationType::Reg3,
  OperationType::Reg3,
  OperationType::Reg3,
  OperationType::Reg3,
  OperationType::Reg3,
  OperationType::Reg3,
  OperationType::ValueOperand,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::ValueOperand,
  OperationType::NoOperands
};
