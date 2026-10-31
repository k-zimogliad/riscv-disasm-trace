#include "riscv/disassembler.hpp"
#include "riscv/types.hpp"

#include <iomanip>
#include <sstream>

namespace riscv {

const char* Disassembler::Mnemonic(InstructionId id) {
    switch (id) {
        case InstructionId::ADD:   return "add";
        case InstructionId::SUB:   return "sub";
        case InstructionId::SLL:   return "sll";
        case InstructionId::SLT:   return "slt";
        case InstructionId::SLTU:  return "sltu";
        case InstructionId::XOR:   return "xor";
        case InstructionId::SRL:   return "srl";
        case InstructionId::SRA:   return "sra";
        case InstructionId::OR:    return "or";
        case InstructionId::AND:   return "and";

        case InstructionId::ADDI:  return "addi";
        case InstructionId::SLTI:  return "slti";
        case InstructionId::SLTIU: return "sltiu";
        case InstructionId::XORI:  return "xori";
        case InstructionId::ORI:   return "ori";
        case InstructionId::ANDI:  return "andi";
        case InstructionId::SLLI:  return "slli";
        case InstructionId::SRLI:  return "srli";
        case InstructionId::SRAI:  return "srai";

        case InstructionId::LW:    return "lw";
        case InstructionId::SW:    return "sw";

        case InstructionId::BEQ:   return "beq";
        case InstructionId::BNE:   return "bne";
        case InstructionId::BLT:   return "blt";
        case InstructionId::BGE:   return "bge";
        case InstructionId::BLTU:  return "bltu";
        case InstructionId::BGEU:  return "bgeu";

        case InstructionId::LUI:   return "lui";
        case InstructionId::AUIPC: return "auipc";

        case InstructionId::JAL:   return "jal";
        case InstructionId::JALR:  return "jalr";

        default:                   return "unknown";
    }
}

std::string Disassembler::RegisterName(uint32_t reg, bool use_abi_names) {
    if (reg >= 32) {
        return "x??";
    }
    if (use_abi_names) {
        return ABI_REG_NAMES[reg];
    }
    return "x" + std::to_string(reg);
}

std::string Disassembler::ToString(const Instruction& inst, bool use_abi_names) {
    if (!inst.is_valid) {
        std::ostringstream out;
        out << ".word 0x" << std::hex << std::setw(8) << std::setfill('0') << inst.raw;
        return out.str();
    }

    const std::string mnem = Mnemonic(inst.id);
    const std::string rd_name  = RegisterName(inst.rd, use_abi_names);
    const std::string rs1_name = RegisterName(inst.rs1, use_abi_names);
    const std::string rs2_name = RegisterName(inst.rs2, use_abi_names);

    std::ostringstream out;

    switch (inst.type) {
        case InstructionType::R:
            out << mnem << " " << rd_name << ", " << rs1_name << ", " << rs2_name;
            break;

        case InstructionType::I:
            if (inst.id == InstructionId::LW || inst.id == InstructionId::JALR) {
                out << mnem << " " << rd_name << ", " << inst.immediate << "(" << rs1_name << ")";
            } else {
                out << mnem << " " << rd_name << ", " << rs1_name << ", " << inst.immediate;
            }
            break;

        case InstructionType::S:
            out << mnem << " " << rs2_name << ", " << inst.immediate << "(" << rs1_name << ")";
            break;

        case InstructionType::B:
            out << mnem << " " << rs1_name << ", " << rs2_name << ", " << inst.immediate;
            break;

        case InstructionType::U: {
            const uint32_t imm20 = (static_cast<uint32_t>(inst.immediate) >> 12) & 0xFFFFF;
            out << mnem << " " << rd_name << ", 0x" << std::hex << imm20;
            break;
        }

        case InstructionType::J:
            out << mnem << " " << rd_name << ", " << inst.immediate;
            break;

        default:
            out << "unknown";
            break;
    }

    return out.str();
}

} // namespace riscv
