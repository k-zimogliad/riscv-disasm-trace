#pragma once

#include <array>
#include <cstdint>

namespace riscv {

using Word = uint32_t;
using Address = uint32_t;
using RegisterValue = uint32_t;

// Формат кодирования инструкции
enum class InstructionType {
    R, I, S, B, U, J, Unknown
};

// Опкоды базового набора RV32I (биты [6:0])
enum class Opcode : uint32_t {
    Lui    = 0b0110111,
    Auipc  = 0b0010111,
    Jal    = 0b1101111,
    Jalr   = 0b1100111,
    Branch = 0b1100011,
    Load   = 0b0000011,
    Store  = 0b0100011,
    OpImm  = 0b0010011,
    Op     = 0b0110011
};

// Идентификаторы инструкций RV32I
enum class InstructionId {
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    LW, JALR, SW,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    LUI, AUIPC, JAL,
    UNKNOWN
};

// Группы для подсчета статистики выполнения
enum class InstructionGroup {
    ALU, Branch, Jump, Memory, Unknown
};

// Счетчики выполненных инструкций по группам
struct ExecutionStats {
    uint64_t total_instructions = 0;
    uint64_t alu_count = 0;
    uint64_t branch_count = 0;
    uint64_t jump_count = 0;
    uint64_t memory_count = 0;
};

// ABI-имена регистров (x0 = zero, x1 = ra, ...)
inline constexpr std::array<const char*, 32> ABI_REG_NAMES = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10","s11","t3", "t4", "t5", "t6"
};

// Определяет группу инструкции по её идентификатору
constexpr InstructionGroup GetInstructionGroup(InstructionId id) noexcept {
    switch (id) {
        case InstructionId::ADD:   case InstructionId::SUB:
        case InstructionId::SLL:   case InstructionId::SLT:
        case InstructionId::SLTU:  case InstructionId::XOR:
        case InstructionId::SRL:   case InstructionId::SRA:
        case InstructionId::OR:    case InstructionId::AND:
        case InstructionId::ADDI:  case InstructionId::SLTI:
        case InstructionId::SLTIU: case InstructionId::XORI:
        case InstructionId::ORI:   case InstructionId::ANDI:
        case InstructionId::SLLI:  case InstructionId::SRLI:
        case InstructionId::SRAI:
        case InstructionId::LUI:   case InstructionId::AUIPC:
            return InstructionGroup::ALU;
        case InstructionId::BEQ:  case InstructionId::BNE:
        case InstructionId::BLT:  case InstructionId::BGE:
        case InstructionId::BLTU: case InstructionId::BGEU:
            return InstructionGroup::Branch;
        case InstructionId::JAL:  case InstructionId::JALR:
            return InstructionGroup::Jump;
        case InstructionId::LW:   case InstructionId::SW:
            return InstructionGroup::Memory;
        default:
            return InstructionGroup::Unknown;
    }
}

} // namespace riscv
