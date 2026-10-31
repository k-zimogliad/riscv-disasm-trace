#pragma once

#include "riscv/types.hpp"

namespace riscv {

struct Instruction {
    Word raw = 0;
    InstructionId id = InstructionId::UNKNOWN;
    InstructionType type = InstructionType::Unknown;

    uint32_t opcode = 0;
    uint32_t rd = 0;
    uint32_t rs1 = 0;
    uint32_t rs2 = 0;
    uint32_t funct3 = 0;
    uint32_t funct7 = 0;

    int32_t immediate = 0;
    bool is_valid = false;
};

} // namespace riscv
