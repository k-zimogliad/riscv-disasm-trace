#pragma once

#include "riscv/types.hpp"

namespace riscv {

// Результат разбора 32-битного машинного слова
struct Instruction {
    Word raw = 0;
    InstructionId id = InstructionId::UNKNOWN;
    InstructionType type = InstructionType::Unknown;

    uint32_t opcode = 0;   // биты [6:0]
    uint32_t rd = 0;       // регистр назначения [11:7]
    uint32_t rs1 = 0;      // первый источник [19:15]
    uint32_t rs2 = 0;      // второй источник [24:20]
    uint32_t funct3 = 0;   // [14:12]
    uint32_t funct7 = 0;   // [31:25]

    int32_t immediate = 0; // знаково-расширенное смещение
    bool is_valid = false;
};

} // namespace riscv
