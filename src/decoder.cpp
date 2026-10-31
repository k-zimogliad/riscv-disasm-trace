#include "riscv/decoder.hpp"

namespace riscv {

namespace {
    constexpr uint32_t OPCODE_OP     = 0x33; // R-тип
    constexpr uint32_t OPCODE_OP_IMM = 0x13; // I-тип (арифметика с immediate)
    constexpr uint32_t OPCODE_LOAD   = 0x03; // I-тип (загрузка из памяти)
    constexpr uint32_t OPCODE_JALR   = 0x67; // I-тип (косвенный переход)
    constexpr uint32_t OPCODE_STORE  = 0x23; // S-тип (сохранение в память)
    constexpr uint32_t OPCODE_BRANCH = 0x63; // B-тип (условные переходы)
    constexpr uint32_t OPCODE_LUI    = 0x37; // U-тип
    constexpr uint32_t OPCODE_AUIPC  = 0x17; // U-тип
    constexpr uint32_t OPCODE_JAL    = 0x6F; // J-тип (безусловный переход)
}

Instruction Decoder::Decode(uint32_t raw_word) {
    const uint32_t opcode = raw_word & 0x7F;
    const uint32_t rd     = (raw_word >> 7)  & 0x1F;
    const uint32_t funct3 = (raw_word >> 12) & 0x07;
    const uint32_t rs1    = (raw_word >> 15) & 0x1F;
    const uint32_t rs2    = (raw_word >> 20) & 0x1F;
    const uint32_t funct7 = (raw_word >> 25) & 0x7F;

    switch (opcode) {
        case OPCODE_OP:
            return DecodeRType(raw_word, rd, funct3, rs1, rs2, funct7);
        case OPCODE_OP_IMM:
        case OPCODE_LOAD:
        case OPCODE_JALR:
            return DecodeIType(raw_word, opcode, rd, funct3, rs1);
        case OPCODE_STORE:
            return DecodeSType(raw_word, funct3, rs1, rs2);
        case OPCODE_BRANCH:
            return DecodeBType(raw_word, funct3, rs1, rs2);
        case OPCODE_LUI:
        case OPCODE_AUIPC:
            return DecodeUType(raw_word, opcode, rd);
        case OPCODE_JAL:
            return DecodeJType(raw_word, rd);
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
    inst.opcode = OPCODE_OP;
    inst.type = InstructionType::R;
    inst.rd = rd;
    inst.rs1 = rs1;
    inst.rs2 = rs2;
    inst.funct3 = funct3;
    inst.funct7 = funct7;
    inst.is_valid = true;

    switch (funct3) {
        case 0x0:
            inst.id = (funct7 == 0x20) ? InstructionId::SUB : InstructionId::ADD;
            break;
        case 0x1: inst.id = InstructionId::SLL;  break;
        case 0x2: inst.id = InstructionId::SLT;  break;
        case 0x3: inst.id = InstructionId::SLTU; break;
        case 0x4: inst.id = InstructionId::XOR;  break;
        case 0x5:
            inst.id = (funct7 == 0x20) ? InstructionId::SRA : InstructionId::SRL;
            break;
        case 0x6: inst.id = InstructionId::OR;   break;
        case 0x7: inst.id = InstructionId::AND;  break;
        default:
            inst.is_valid = false;
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
    inst.is_valid = true;

    // В I-типе смещение хранится в битах [31:20]
    inst.immediate = SignExtend((raw >> 20) & 0xFFF, 12);

    if (opcode == OPCODE_OP_IMM) {
        const uint32_t f7 = (raw >> 25) & 0x7F;
        switch (funct3) {
            case 0x0: inst.id = InstructionId::ADDI;  break;
            case 0x2: inst.id = InstructionId::SLTI;  break;
            case 0x3: inst.id = InstructionId::SLTIU; break;
            case 0x4: inst.id = InstructionId::XORI;  break;
            case 0x6: inst.id = InstructionId::ORI;   break;
            case 0x7: inst.id = InstructionId::ANDI;  break;
            case 0x1:
                inst.id = InstructionId::SLLI;
                inst.funct7 = f7;
                inst.immediate = static_cast<int32_t>((raw >> 20) & 0x1F);
                break;
            case 0x5:
                inst.funct7 = f7;
                inst.id = (f7 == 0x20) ? InstructionId::SRAI : InstructionId::SRLI;
                inst.immediate = static_cast<int32_t>((raw >> 20) & 0x1F);
                break;
            default:
                inst.is_valid = false;
                break;
        }
    } else if (opcode == OPCODE_LOAD) {
        if (funct3 == 0x2) {
            inst.id = InstructionId::LW;
        } else {
            inst.is_valid = false;
        }
    } else if (opcode == OPCODE_JALR) {
        inst.id = InstructionId::JALR;
    }

    return inst;
}

Instruction Decoder::DecodeSType(uint32_t raw, uint32_t funct3,
                                uint32_t rs1, uint32_t rs2) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = OPCODE_STORE;
    inst.type = InstructionType::S;
    inst.rs1 = rs1;
    inst.rs2 = rs2;
    inst.funct3 = funct3;
    inst.is_valid = true;

    // Смещение разделено на две части: [31:25] и [11:7]
    const uint32_t imm_11_5 = (raw >> 25) & 0x7F;
    const uint32_t imm_4_0  = (raw >> 7)  & 0x1F;
    const uint32_t imm = (imm_11_5 << 5) | imm_4_0;
    inst.immediate = SignExtend(imm, 12);

    if (funct3 == 0x2) {
        inst.id = InstructionId::SW;
    } else {
        inst.is_valid = false;
    }
    return inst;
}

Instruction Decoder::DecodeBType(uint32_t raw, uint32_t funct3,
                                uint32_t rs1, uint32_t rs2) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = OPCODE_BRANCH;
    inst.type = InstructionType::B;
    inst.rs1 = rs1;
    inst.rs2 = rs2;
    inst.funct3 = funct3;
    inst.is_valid = true;

    // Сборка 13-битного смещения B-типа (младший бит всегда 0)
    const uint32_t bit_12    = (raw >> 31) & 0x01;
    const uint32_t bit_11    = (raw >> 7)  & 0x01;
    const uint32_t bits_10_5 = (raw >> 25) & 0x3F;
    const uint32_t bits_4_1  = (raw >> 8)  & 0x0F;

    const uint32_t imm = (bit_12 << 12) | (bit_11 << 11) |
                         (bits_10_5 << 5) | (bits_4_1 << 1);
    inst.immediate = SignExtend(imm, 13);

    switch (funct3) {
        case 0x0: inst.id = InstructionId::BEQ;  break;
        case 0x1: inst.id = InstructionId::BNE;  break;
        case 0x4: inst.id = InstructionId::BLT;  break;
        case 0x5: inst.id = InstructionId::BGE;  break;
        case 0x6: inst.id = InstructionId::BLTU; break;
        case 0x7: inst.id = InstructionId::BGEU; break;
        default:
            inst.is_valid = false;
            break;
    }
    return inst;
}

Instruction Decoder::DecodeUType(uint32_t raw, uint32_t opcode, uint32_t rd) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = opcode;
    inst.type = InstructionType::U;
    inst.rd = rd;
    inst.is_valid = true;

    // Младшие 12 бит зануляются
    inst.immediate = static_cast<int32_t>(raw & 0xFFFFF000);
    inst.id = (opcode == OPCODE_LUI) ? InstructionId::LUI : InstructionId::AUIPC;

    return inst;
}

Instruction Decoder::DecodeJType(uint32_t raw, uint32_t rd) {
    Instruction inst;
    inst.raw = raw;
    inst.opcode = OPCODE_JAL;
    inst.type = InstructionType::J;
    inst.rd = rd;
    inst.is_valid = true;

    // Сборка 21-битного смещения J-типа
    const uint32_t bit_20    = (raw >> 31) & 0x01;
    const uint32_t bits_19_12 = (raw >> 12) & 0xFF;
    const uint32_t bit_11    = (raw >> 20) & 0x01;
    const uint32_t bits_10_1  = (raw >> 21) & 0x3FF;

    const uint32_t imm = (bit_20 << 20) | (bits_19_12 << 12) |
                         (bit_11 << 11) | (bits_10_1 << 1);
    inst.immediate = SignExtend(imm, 21);
    inst.id = InstructionId::JAL;

    return inst;
}

int32_t Decoder::SignExtend(uint32_t value, unsigned bit_width) {
    const uint32_t sign_bit = 1u << (bit_width - 1);
    if (value & sign_bit) {
        return static_cast<int32_t>(value | (~0u << bit_width));
    }
    return static_cast<int32_t>(value);
}

} // namespace riscv
