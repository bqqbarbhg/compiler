#pragma once

enum instruction_set
{
	InstructionSetUnknown,
	InstructionSetX86,
	InstructionSetX86_64,
	InstructionSetARM,
	InstructionSetARM64,
};

enum target_flags
{
	TargetFlag64Bit = 1 << 0,
};

struct target_info
{
	instruction_set ISA;
	uint32_t Flags;
};

