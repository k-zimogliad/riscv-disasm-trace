#pragma once

#include "riscv/instruction.hpp"
#include <cstdint>

namespace riscv {

// Декодер инструкций RISC-V (набор RV32I)
class Decoder {
public:
    static Instruction Decode(uint32_t raw_word);

private:
    static Instruction DecodeRType(uint32_t raw, uint32_t rd, uint32_t funct3,
                                   uint32_t rs1, uint32_t rs2, uint32_t funct7);
    static Instruction DecodeIType(uint32_t raw, uint32_t opcode, uint32_t rd,
                                   uint32_t funct3, uint32_t rs1);
    static Instruction DecodeSType(uint32_t raw, uint32_t funct3,
                                   uint32_t rs1, uint32_t rs2);
    static Instruction DecodeBType(uint32_t raw, uint32_t funct3,
                                   uint32_t rs1, uint32_t rs2);
    static Instruction DecodeUType(uint32_t raw, uint32_t opcode, uint32_t rd);
    static Instruction DecodeJType(uint32_t raw, uint32_t rd);

    static int32_t SignExtend(uint32_t value, unsigned bit_width);
};

} // namespace riscv
