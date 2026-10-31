#pragma once

#include <array>
#include <cstdint>

namespace riscv {

using Word = uint32_t;
using Address = uint32_t;
using RegisterValue = uint32_t;

enum class InstructionType {
    R, I, S, B, U, J, Unknown
};

enum class Opcode : uint32_t {
    Load    = 0x03,
    MiscMem = 0x0F,
    OpImm   = 0x13,
    Auipc   = 0x17,
    Store   = 0x23,
    Op      = 0x33,
    Lui     = 0x37,
    Branch  = 0x63,
    Jalr    = 0x67,
    Jal     = 0x6F,
    System  = 0x73
};

enum class InstructionId {
    // R-type
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // I-type (ALU)
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    // I-type (Loads)
    LB, LH, LW, LBU, LHU,
    // I-type (Jump & System)
    JALR, ECALL, EBREAK, FENCE,
    // S-type (Stores)
    SB, SH, SW,
    // B-type (Branches)
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // U-type
    LUI, AUIPC,
    // J-type
    JAL,

    UNKNOWN
};

enum class InstructionGroup {
    ALU,
    Branch,
    Jump,
    Memory,
    System,
    Unknown
};

struct ExecutionStats {
    uint64_t total_instructions = 0;
    uint64_t alu_count = 0;
    uint64_t branch_count = 0;
    uint64_t jump_count = 0;
    uint64_t memory_count = 0;
    uint64_t system_count = 0;
};

inline constexpr std::array<const char*, 32> ABI_REG_NAMES = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10","s11","t3", "t4", "t5", "t6"
};

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

        case InstructionId::JAL:
        case InstructionId::JALR:
            return InstructionGroup::Jump;

        case InstructionId::LB:  case InstructionId::LH:
        case InstructionId::LW:  case InstructionId::LBU:
        case InstructionId::LHU: case InstructionId::SB:
        case InstructionId::SH:  case InstructionId::SW:
            return InstructionGroup::Memory;

        case InstructionId::ECALL:
        case InstructionId::EBREAK:
        case InstructionId::FENCE:
            return InstructionGroup::System;

        default:
            return InstructionGroup::Unknown;
    }
}

} // namespace riscv
