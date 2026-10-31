#include "riscv/decoder.hpp"
#include "riscv/types.hpp"

namespace riscv {

Instruction Decoder::Decode(uint32_t raw_word) {
    const uint32_t opcode = raw_word & 0x7F;
    const uint32_t rd     = (raw_word >> 7)  & 0x1F;
    const uint32_t funct3 = (raw_word >> 12) & 0x07;
    const uint32_t rs1    = (raw_word >> 15) & 0x1F;
    const uint32_t rs2    = (raw_word >> 20) & 0x1F;
    const uint32_t funct7 = (raw_word >> 25) & 0x7F;

    switch (static_cast<Opcode>(opcode)) {
        case Opcode::Op:
            return DecodeRType(raw_word, rd, funct3, rs1, rs2, funct7);
        case Opcode::OpImm:
        case Opcode::Load:
        case Opcode::Jalr:
            return DecodeIType(raw_word, opcode, rd, funct3, rs1);
        case Opcode::Store:
            return DecodeSType(raw_word, funct3, rs1, rs2);
        case Opcode::Branch:
            return DecodeBType(raw_word, funct3, rs1, rs2);
        case Opcode::Lui:
        case Opcode::Auipc:
            return DecodeUType(raw_word, opcode, rd);
        case Opcode::Jal:
            return DecodeJType(raw_word, rd);
        case Opcode::System:
            return DecodeSystem(raw_word, rd, funct3, rs1, funct7);
        case Opcode::MiscMem:
            return DecodeMiscMem(raw_word, rd, funct3, rs1);
        default: {
            Instruction inst;
            inst.raw = raw_word;
            inst.opcode = opcode;
            inst.is_valid = false;
            return inst;
        }
    }
}

Instruction Decoder::DecodeRType(uint32_t raw, uint32_t rd, uint32_t funct3,
                                 uint32_t rs1, uint32_t rs2, uint32_t funct7) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = static_cast<uint32_t>(Opcode::Op);
    inst.type = InstructionType::R;
    inst.rd = rd;
    inst.rs1 = rs1;
    inst.rs2 = rs2;
    inst.funct3 = funct3;
    inst.funct7 = funct7;

    switch (funct3) {
        case 0x0:
            if (funct7 == 0x00) {
                inst.id = InstructionId::ADD;
                inst.is_valid = true;
            } else if (funct7 == 0x20) {
                inst.id = InstructionId::SUB;
                inst.is_valid = true;
            }
            break;
        case 0x1:
            if (funct7 == 0x00) {
                inst.id = InstructionId::SLL;
                inst.is_valid = true;
            }
            break;
        case 0x2:
            if (funct7 == 0x00) {
                inst.id = InstructionId::SLT;
                inst.is_valid = true;
            }
            break;
        case 0x3:
            if (funct7 == 0x00) {
                inst.id = InstructionId::SLTU;
                inst.is_valid = true;
            }
            break;
        case 0x4:
            if (funct7 == 0x00) {
                inst.id = InstructionId::XOR;
                inst.is_valid = true;
            }
            break;
        case 0x5:
            if (funct7 == 0x00) {
                inst.id = InstructionId::SRL;
                inst.is_valid = true;
            } else if (funct7 == 0x20) {
                inst.id = InstructionId::SRA;
                inst.is_valid = true;
            }
            break;
        case 0x6:
            if (funct7 == 0x00) {
                inst.id = InstructionId::OR;
                inst.is_valid = true;
            }
            break;
        case 0x7:
            if (funct7 == 0x00) {
                inst.id = InstructionId::AND;
                inst.is_valid = true;
            }
            break;
        default:
            break;
    }
    return inst;
}

Instruction Decoder::DecodeIType(uint32_t raw, uint32_t opcode, uint32_t rd,
                                 uint32_t funct3, uint32_t rs1) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = opcode;
    inst.type = InstructionType::I;
    inst.rd = rd;
    inst.rs1 = rs1;
    inst.funct3 = funct3;

    const uint32_t imm12 = (raw >> 20) & 0xFFF;
    inst.immediate = SignExtend(imm12, 12);

    if (opcode == static_cast<uint32_t>(Opcode::OpImm)) {
        const uint32_t f7 = (raw >> 25) & 0x7F;
        const uint32_t shamt = (raw >> 20) & 0x1F;

        switch (funct3) {
            case 0x0: inst.id = InstructionId::ADDI;  inst.is_valid = true; break;
            case 0x2: inst.id = InstructionId::SLTI;  inst.is_valid = true; break;
            case 0x3: inst.id = InstructionId::SLTIU; inst.is_valid = true; break;
            case 0x4: inst.id = InstructionId::XORI;  inst.is_valid = true; break;
            case 0x6: inst.id = InstructionId::ORI;   inst.is_valid = true; break;
            case 0x7: inst.id = InstructionId::ANDI;  inst.is_valid = true; break;
            case 0x1:
                if (f7 == 0x00) {
                    inst.id = InstructionId::SLLI;
                    inst.funct7 = f7;
                    inst.immediate = static_cast<int32_t>(shamt);
                    inst.is_valid = true;
                }
                break;
            case 0x5:
                if (f7 == 0x00) {
                    inst.id = InstructionId::SRLI;
                    inst.funct7 = f7;
                    inst.immediate = static_cast<int32_t>(shamt);
                    inst.is_valid = true;
                } else if (f7 == 0x20) {
                    inst.id = InstructionId::SRAI;
                    inst.funct7 = f7;
                    inst.immediate = static_cast<int32_t>(shamt);
                    inst.is_valid = true;
                }
                break;
            default:
                break;
        }
    } else if (opcode == static_cast<uint32_t>(Opcode::Load)) {
        switch (funct3) {
            case 0x0: inst.id = InstructionId::LB;  inst.is_valid = true; break;
            case 0x1: inst.id = InstructionId::LH;  inst.is_valid = true; break;
            case 0x2: inst.id = InstructionId::LW;  inst.is_valid = true; break;
            case 0x4: inst.id = InstructionId::LBU; inst.is_valid = true; break;
            case 0x5: inst.id = InstructionId::LHU; inst.is_valid = true; break;
            default: break;
        }
    } else if (opcode == static_cast<uint32_t>(Opcode::Jalr)) {
        if (funct3 == 0x0) {
            inst.id = InstructionId::JALR;
            inst.is_valid = true;
        }
    }

    return inst;
}

Instruction Decoder::DecodeSType(uint32_t raw, uint32_t funct3,
                                 uint32_t rs1, uint32_t rs2) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = static_cast<uint32_t>(Opcode::Store);
    inst.type = InstructionType::S;
    inst.rs1 = rs1;
    inst.rs2 = rs2;
    inst.funct3 = funct3;

    const uint32_t imm = ((raw >> 25) & 0x7F) << 5 | ((raw >> 7) & 0x1F);
    inst.immediate = SignExtend(imm, 12);

    switch (funct3) {
        case 0x0: inst.id = InstructionId::SB; inst.is_valid = true; break;
        case 0x1: inst.id = InstructionId::SH; inst.is_valid = true; break;
        case 0x2: inst.id = InstructionId::SW; inst.is_valid = true; break;
        default: break;
    }
    return inst;
}

Instruction Decoder::DecodeBType(uint32_t raw, uint32_t funct3,
                                 uint32_t rs1, uint32_t rs2) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = static_cast<uint32_t>(Opcode::Branch);
    inst.type = InstructionType::B;
    inst.rs1 = rs1;
    inst.rs2 = rs2;
    inst.funct3 = funct3;

    const uint32_t imm = ((raw >> 31) & 0x1) << 12 |
                         ((raw >> 7)  & 0x1) << 11 |
                         ((raw >> 25) & 0x3F) << 5 |
                         ((raw >> 8)  & 0xF) << 1;
    inst.immediate = SignExtend(imm, 13);

    switch (funct3) {
        case 0x0: inst.id = InstructionId::BEQ;  inst.is_valid = true; break;
        case 0x1: inst.id = InstructionId::BNE;  inst.is_valid = true; break;
        case 0x4: inst.id = InstructionId::BLT;  inst.is_valid = true; break;
        case 0x5: inst.id = InstructionId::BGE;  inst.is_valid = true; break;
        case 0x6: inst.id = InstructionId::BLTU; inst.is_valid = true; break;
        case 0x7: inst.id = InstructionId::BGEU; inst.is_valid = true; break;
        default: break;
    }
    return inst;
}

Instruction Decoder::DecodeUType(uint32_t raw, uint32_t opcode, uint32_t rd) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = opcode;
    inst.type = InstructionType::U;
    inst.rd = rd;
    inst.immediate = static_cast<int32_t>(raw & 0xFFFFF000);
    inst.id = (opcode == static_cast<uint32_t>(Opcode::Lui)) ? InstructionId::LUI : InstructionId::AUIPC;
    inst.is_valid = true;
    return inst;
}

Instruction Decoder::DecodeJType(uint32_t raw, uint32_t rd) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = static_cast<uint32_t>(Opcode::Jal);
    inst.type = InstructionType::J;
    inst.rd = rd;

    const uint32_t imm = ((raw >> 31) & 0x1) << 20 |
                         ((raw >> 12) & 0xFF) << 12 |
                         ((raw >> 20) & 0x1) << 11 |
                         ((raw >> 21) & 0x3FF) << 1;
    inst.immediate = SignExtend(imm, 21);
    inst.id = InstructionId::JAL;
    inst.is_valid = true;
    return inst;
}

Instruction Decoder::DecodeSystem(uint32_t raw, uint32_t rd, uint32_t funct3,
                                  uint32_t rs1, uint32_t funct7) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = static_cast<uint32_t>(Opcode::System);
    inst.type = InstructionType::I;
    inst.rd = rd;
    inst.rs1 = rs1;
    inst.funct3 = funct3;
    inst.funct7 = funct7;

    if (funct3 == 0x0 && rd == 0 && rs1 == 0) {
        if (raw == 0x00000073) {
            inst.id = InstructionId::ECALL;
            inst.is_valid = true;
        } else if (raw == 0x00100073) {
            inst.id = InstructionId::EBREAK;
            inst.is_valid = true;
        }
    }
    return inst;
}

Instruction Decoder::DecodeMiscMem(uint32_t raw, uint32_t rd, uint32_t funct3,
                                   uint32_t rs1) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = static_cast<uint32_t>(Opcode::MiscMem);
    inst.type = InstructionType::I;
    inst.rd = rd;
    inst.rs1 = rs1;
    inst.funct3 = funct3;

    if (funct3 == 0x0) {
        inst.id = InstructionId::FENCE;
        inst.is_valid = true;
    }
    return inst;
}

int32_t Decoder::SignExtend(uint32_t value, unsigned bit_width) {
    if (bit_width == 0 || bit_width >= 32) {
        return static_cast<int32_t>(value);
    }
    const unsigned shift = 32 - bit_width;
    return static_cast<int32_t>(value << shift) >> shift;
}

} // namespace riscv
