#ifndef MC3_UTILS_HPP
#define MC3_UTILS_HPP

enum class Opcode
{
  None = -1,
  OrVal,
  AndVal,
  XorVal,
  //LshVal_Removed,
  //RshVal_Removed,
  AddVal,
  SubVal,
  SingleOp,
  OrReg,
  AndReg,
  XorReg,
  LshReg,
  RshReg,
  LrotReg,
  RrotReg,
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
  [(int)Opcode::OrVal] = OperationType::ValueOperand,
  [(int)Opcode::AndVal] = OperationType::ValueOperand,
  [(int)Opcode::XorVal] = OperationType::ValueOperand,
  //[Opcode::LshVal_Removed] = OperationType::ValueOperand,
  //[Opcode::LshVal_Removed] = OperationType::ValueOperand,
  [(int)Opcode::AddVal] = OperationType::ValueOperand,
  [(int)Opcode::SubVal] = OperationType::ValueOperand,
  [(int)Opcode::SingleOp] = OperationType::OneOperand,
  [(int)Opcode::OrReg] = OperationType::Reg3,
  [(int)Opcode::AndReg] = OperationType::Reg3,
  [(int)Opcode::XorReg] = OperationType::Reg3,
  [(int)Opcode::LshReg] = OperationType::Reg3,
  [(int)Opcode::RshReg] = OperationType::Reg3,
  [(int)Opcode::LrotReg] = OperationType::Reg3,
  [(int)Opcode::RrotReg] = OperationType::Reg3,
  [(int)Opcode::AddReg] = OperationType::Reg3,
  [(int)Opcode::SubReg] = OperationType::Reg3,
  [(int)Opcode::SetVal] = OperationType::ValueOperand,
  [(int)Opcode::LodB] = OperationType::RegValue,
  [(int)Opcode::LodW] = OperationType::RegValue,
  [(int)Opcode::StrB] = OperationType::RegValue,
  [(int)Opcode::StrW] = OperationType::RegValue,
  [(int)Opcode::JmpZ] = OperationType::ValueOperand,
  [(int)Opcode::JmpNz] = OperationType::ValueOperand,
  [(int)Opcode::JmpC] = OperationType::ValueOperand,
  [(int)Opcode::JmpNc] = OperationType::ValueOperand,
  [(int)Opcode::JmpS] = OperationType::ValueOperand,
  [(int)Opcode::JmpNs] = OperationType::ValueOperand,
  [(int)Opcode::JmpO] = OperationType::ValueOperand,
  [(int)Opcode::JmpNo] = OperationType::ValueOperand,
  [(int)Opcode::OpOnly] = OperationType::NoOperands
};

#endif // MC3_UTILS_HPP
