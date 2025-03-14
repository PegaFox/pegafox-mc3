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
  SetReg,
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
  RegValue,
  RegValueExt
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
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::RegValue,
  OperationType::ValueOperand,
  OperationType::RegValueExt,
  OperationType::RegValueExt,
  OperationType::RegValueExt,
  OperationType::RegValueExt,
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