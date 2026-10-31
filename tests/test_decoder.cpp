#include "riscv/decoder.hpp"
#include "riscv/disassembler.hpp"

#include <gtest/gtest.h>

using namespace riscv;

// Тест декодирования NOP (addi x0, x0, 0)
TEST(DecoderTest, DecodeNop) {
    const Instruction inst = Decoder::Decode(0x00000013);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.type, InstructionType::I);
    EXPECT_EQ(inst.rd, 0u);
    EXPECT_EQ(inst.rs1, 0u);
    EXPECT_EQ(inst.immediate, 0);
}

// 0x02A00513 -> addi a0, zero, 42
TEST(DecoderTest, DecodeAddiPositiveImm) {
    const Instruction inst = Decoder::Decode(0x02A00513);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.type, InstructionType::I);
    EXPECT_EQ(inst.rd, 10u);   // a0
    EXPECT_EQ(inst.rs1, 0u);   // zero
    EXPECT_EQ(inst.immediate, 42);
}

// 0x00B50533 -> add a0, a0, a1
TEST(DecoderTest, DecodeAdd) {
    const Instruction inst = Decoder::Decode(0x00B50533);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADD);
    EXPECT_EQ(inst.type, InstructionType::R);
    EXPECT_EQ(inst.rd, 10u);   // a0
    EXPECT_EQ(inst.rs1, 10u);  // a0
    EXPECT_EQ(inst.rs2, 11u);  // a1
    EXPECT_EQ(inst.funct7, 0u);
}

// 0xFEB50CE3 -> beq a0, a1, -8
TEST(DecoderTest, DecodeBEQ_NegativeOffset) {
    const Instruction inst = Decoder::Decode(0xFEB50CE3);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::BEQ);
    EXPECT_EQ(inst.type, InstructionType::B);
    EXPECT_EQ(inst.rs1, 10u);  // a0
    EXPECT_EQ(inst.rs2, 11u);  // a1
    EXPECT_EQ(inst.immediate, -8);
}

// 0x008000EF -> jal ra, 8
TEST(DecoderTest, DecodeJAL) {
    const Instruction inst = Decoder::Decode(0x008000EF);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::JAL);
    EXPECT_EQ(inst.type, InstructionType::J);
    EXPECT_EQ(inst.rd, 1u);    // ra
    EXPECT_EQ(inst.immediate, 8);
}

// Проверка знакового расширения отрицательного immediate в I-типе (-1)
TEST(DecoderTest, SignExtendNegativeAddi) {
    const Instruction inst = Decoder::Decode(0xFFF00513);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.immediate, -1);
}

// Минимальное 12-битное отрицательное число (-2048)
TEST(DecoderTest, SignExtendMinAddi) {
    const Instruction inst = Decoder::Decode(0x80000513);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::ADDI);
    EXPECT_EQ(inst.immediate, -2048);
}

// Знаковое расширение для ветвлений B-типа (смещение -4)
TEST(DecoderTest, SignExtendBType) {
    const Instruction inst = Decoder::Decode(0xFE000EE3);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::BEQ);
    EXPECT_EQ(inst.immediate, -4);
}

// Декодирование R-типа: sub a0, a1, a2
TEST(DecoderTest, DecodeSub) {
    const Instruction inst = Decoder::Decode(0x40C58533);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::SUB);
    EXPECT_EQ(inst.rd, 10u);
    EXPECT_EQ(inst.rs1, 11u);
    EXPECT_EQ(inst.rs2, 12u);
    EXPECT_EQ(inst.funct7, 0x20u);
}

// Декодирование U-типа: lui
TEST(DecoderTest, DecodeLui) {
    const Instruction inst = Decoder::Decode(0xDEADB537);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::LUI);
    EXPECT_EQ(inst.rd, 10u);
    EXPECT_EQ(static_cast<uint32_t>(inst.immediate), 0xDEADB000u);
}

// Декодирование S-типа: sw
TEST(DecoderTest, DecodeSwPositiveOffset) {
    const Instruction inst = Decoder::Decode(0x00B52823);

    EXPECT_TRUE(inst.is_valid);
    EXPECT_EQ(inst.id, InstructionId::SW);
    EXPECT_EQ(inst.rs1, 10u);
    EXPECT_EQ(inst.rs2, 11u);
    EXPECT_EQ(inst.immediate, 16);
}

// Тесты форматирования дизассемблера
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

TEST(DisassemblerTest, InvalidInstruction) {
    const Instruction inst = Decoder::Decode(0xFFFFFFFF);
    EXPECT_EQ(Disassembler::ToString(inst, true), ".word 0xffffffff");
}
