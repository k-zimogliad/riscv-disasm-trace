#pragma once

#include "riscv/instruction.hpp"
#include <string>

namespace riscv {

class Disassembler {
public:
    static std::string ToString(const Instruction& inst, bool use_abi_names = true);
    static std::string RegisterName(uint32_t reg, bool use_abi_names = true);

private:
    static const char* Mnemonic(InstructionId id);
};

} // namespace riscv
