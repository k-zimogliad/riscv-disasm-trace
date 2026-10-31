#include "riscv/decoder.hpp"
#include "riscv/disassembler.hpp"

#include <gtest/gtest.h>

using namespace riscv;

TEST(DecoderTest, DecodeNop) {
    const Instruction inst = Decoder::Decode(0x00000013);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.type, InstructionType::I);
    EXPECT_EQ(inst.rd, 0u);
    EXPECT_EQ(inst.rs1, 0u);
    EXPECT_EQ(inst.immediate, 0);
}

TEST(DecoderTest, DecodeAddiPositiveImm) {
    const Instruction inst = Decoder::Decode(0x02A00513);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.type, InstructionType::I);
    EXPECT_EQ(inst.rd, 10u);
    EXPECT_EQ(inst.rs1, 0u);
    EXPECT_EQ(inst.immediate, 42);
}

TEST(DecoderTest, DecodeAdd) {
    const Instruction inst = Decoder::Decode(0x00B50533);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADD);
    EXPECT_EQ(inst.type, InstructionType::R);
    EXPECT_EQ(inst.rd, 10u);
    EXPECT_EQ(inst.rs1, 10u);
    EXPECT_EQ(inst.rs2, 11u);
    EXPECT_EQ(inst.funct7, 0u);
}

TEST(DecoderTest, DecodeBEQ_NegativeOffset) {
    const Instruction inst = Decoder::Decode(0xFEB50CE3);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::BEQ);
    EXPECT_EQ(inst.type, InstructionType::B);
    EXPECT_EQ(inst.rs1, 10u);
    EXPECT_EQ(inst.rs2, 11u);
    EXPECT_EQ(inst.immediate, -8);
}

TEST(DecoderTest, DecodeJAL) {
    const Instruction inst = Decoder::Decode(0x008000EF);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::JAL);
    EXPECT_EQ(inst.type, InstructionType::J);
    EXPECT_EQ(inst.rd, 1u);
    EXPECT_EQ(inst.immediate, 8);
}

TEST(DecoderTest, SignExtendNegativeAddi) {
    const Instruction inst = Decoder::Decode(0xFFF00513);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.immediate, -1);
}

TEST(DecoderTest, SignExtendMinAddi) {
    const Instruction inst = Decoder::Decode(0x80000513);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.immediate, -2048);
}

TEST(DecoderTest, SignExtendBType) {
    const Instruction inst = Decoder::Decode(0xFE000EE3);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::BEQ);
    EXPECT_EQ(inst.immediate, -4);
}

TEST(DecoderTest, DecodeSub) {
    const Instruction inst = Decoder::Decode(0x40C58533);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::SUB);
    EXPECT_EQ(inst.rd, 10u);
    EXPECT_EQ(inst.rs1, 11u);
    EXPECT_EQ(inst.rs2, 12u);
    EXPECT_EQ(inst.funct7, 0x20u);
}

TEST(DecoderTest, DecodeLui) {
    const Instruction inst = Decoder::Decode(0x12345537);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::LUI);
    EXPECT_EQ(inst.rd, 10u);
    EXPECT_EQ(static_cast<uint32_t>(inst.immediate), 0x12345000u);
}

TEST(DecoderTest, DecodeLoadsAndStores) {
    // lb x10, 4(x11) -> opcode=0x03, funct3=0x0
    const Instruction lb_inst = Decoder::Decode(0x00458503);
    EXPECT_TRUE(lb_inst.is_valid);
    EXPECT_EQ(lb_inst.id, InstructionId::LB);
    EXPECT_EQ(lb_inst.rd, 10u);
    EXPECT_EQ(lb_inst.rs1, 11u);
    EXPECT_EQ(lb_inst.immediate, 4);

    // lbu x10, 4(x11) -> opcode=0x03, funct3=0x4
    const Instruction lbu_inst = Decoder::Decode(0x0045C503);
    EXPECT_TRUE(lbu_inst.is_valid);
    EXPECT_EQ(lbu_inst.id, InstructionId::LBU);

    // lh x10, 2(x11) -> opcode=0x03, funct3=0x1
    const Instruction lh_inst = Decoder::Decode(0x00259503);
    EXPECT_TRUE(lh_inst.is_valid);
    EXPECT_EQ(lh_inst.id, InstructionId::LH);

    // lhu x10, 2(x11) -> opcode=0x03, funct3=0x5
    const Instruction lhu_inst = Decoder::Decode(0x0025D503);
    EXPECT_TRUE(lhu_inst.is_valid);
    EXPECT_EQ(lhu_inst.id, InstructionId::LHU);

    // sb x12, 1(x11) -> opcode=0x23, funct3=0x0
    const Instruction sb_inst = Decoder::Decode(0x00C580A3);
    EXPECT_TRUE(sb_inst.is_valid);
    EXPECT_EQ(sb_inst.id, InstructionId::SB);

    // sh x12, 2(x11) -> opcode=0x23, funct3=0x1
    const Instruction sh_inst = Decoder::Decode(0x00C59123);
    EXPECT_TRUE(sh_inst.is_valid);
    EXPECT_EQ(sh_inst.id, InstructionId::SH);

    // sw x11, 16(x10) -> opcode=0x23, funct3=0x2
    const Instruction sw_inst = Decoder::Decode(0x00B52823);
    EXPECT_TRUE(sw_inst.is_valid);
    EXPECT_EQ(sw_inst.id, InstructionId::SW);
    EXPECT_EQ(sw_inst.rs1, 10u);
    EXPECT_EQ(sw_inst.rs2, 11u);
    EXPECT_EQ(sw_inst.immediate, 16);
}

TEST(DecoderTest, DecodeShiftValidation) {
    // slli x10, x11, 5 with funct7 != 0 should be invalid
    // 0x40559513 has funct7=0x20 with funct3=1
    const Instruction invalid_slli = Decoder::Decode(0x40559513);
    EXPECT_FALSE(invalid_slli.is_valid);

    // valid slli: 0x00559513 (funct7=0)
    const Instruction valid_slli = Decoder::Decode(0x00559513);
    EXPECT_TRUE(valid_slli.is_valid);
    EXPECT_EQ(valid_slli.id, InstructionId::SLLI);
    EXPECT_EQ(valid_slli.immediate, 5);
}

TEST(DecoderTest, DecodeJALRValidation) {
    // jalr x1, 0(x2) with funct3=0 -> valid (0x000100E7)
    const Instruction jalr_inst = Decoder::Decode(0x000100E7);
    EXPECT_TRUE(jalr_inst.is_valid);
    EXPECT_EQ(jalr_inst.id, InstructionId::JALR);

    // jalr with non-zero funct3 -> invalid
    const Instruction invalid_jalr = Decoder::Decode(0x000110E7);
    EXPECT_FALSE(invalid_jalr.is_valid);
}

TEST(DecoderTest, DecodeSystemAndSync) {
    // ecall: 0x00000073
    const Instruction ecall_inst = Decoder::Decode(0x00000073);
    EXPECT_TRUE(ecall_inst.is_valid);
    EXPECT_EQ(ecall_inst.id, InstructionId::ECALL);

    // ebreak: 0x00100073
    const Instruction ebreak_inst = Decoder::Decode(0x00100073);
    EXPECT_TRUE(ebreak_inst.is_valid);
    EXPECT_EQ(ebreak_inst.id, InstructionId::EBREAK);

    // fence: 0x0000000F
    const Instruction fence_inst = Decoder::Decode(0x0000000F);
    EXPECT_TRUE(fence_inst.is_valid);
    EXPECT_EQ(fence_inst.id, InstructionId::FENCE);
}

TEST(DisassemblerTest, NopString) {
    const Instruction inst = Decoder::Decode(0x00000013);
    EXPECT_EQ(Disassembler::ToString(inst, true),  "addi zero, zero, 0");
    EXPECT_EQ(Disassembler::ToString(inst, false), "addi x0, x0, 0");
}

TEST(DisassemblerTest, JalString) {
    const Instruction inst = Decoder::Decode(0x008000EF);
    EXPECT_EQ(Disassembler::ToString(inst, true), "jal ra, 8");
}

TEST(DisassemblerTest, BranchString) {
    const Instruction inst = Decoder::Decode(0xFEB50CE3);
    EXPECT_EQ(Disassembler::ToString(inst, true), "beq a0, a1, -8");
}

TEST(DisassemblerTest, SystemAndMemoryStrings) {
    EXPECT_EQ(Disassembler::ToString(Decoder::Decode(0x00100073), true), "ebreak");
    EXPECT_EQ(Disassembler::ToString(Decoder::Decode(0x00000073), true), "ecall");
    EXPECT_EQ(Disassembler::ToString(Decoder::Decode(0x0000000F), true), "fence");
    EXPECT_EQ(Disassembler::ToString(Decoder::Decode(0x00458503), true), "lb a0, 4(a1)");
    EXPECT_EQ(Disassembler::ToString(Decoder::Decode(0x00C580A3), true), "sb a2, 1(a1)");
}

TEST(DisassemblerTest, InvalidInstruction) {
    const Instruction inst = Decoder::Decode(0xFFFFFFFF);
    EXPECT_EQ(Disassembler::ToString(inst, true), ".word 0xffffffff");
}
