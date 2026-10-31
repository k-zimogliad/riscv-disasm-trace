#include "riscv/emulator.hpp"
#include "riscv/decoder.hpp"

#include <gtest/gtest.h>
#include <vector>

using namespace riscv;

// Тест 1: Вычисление суммы чисел от 1 до 5 в цикле (1 + 2 + 3 + 4 + 5 = 15)
//
// Логика:
//   x10 = 6  (граница)
//   x11 = 0  (сумма)
//   x12 = 1  (счетчик i)
// loop:
//   if (x12 >= x10) goto end;
//   x11 += x12;
//   x12 += 1;
//   goto loop;
// end:
TEST(EmulatorTest, SumOneToFive) {
    const std::vector<uint32_t> program = {
        0x00600513,  // addi x10, x0, 6
        0x00000593,  // addi x11, x0, 0
        0x00100613,  // addi x12, x0, 1
        0x00A65863,  // bge  x12, x10, 16 -> выход из цикла
        0x00C585B3,  // add  x11, x11, x12
        0x00160613,  // addi x12, x12, 1
        0xFF5FF06F   // jal  x0, -12      -> возврат к проверке bge
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(200);

    // Проверяем результат суммы
    EXPECT_EQ(emu.GetRegister(11), 15u);
    EXPECT_EQ(emu.GetRegister(12), 6u);

    // Проверяем сбор статистики
    const ExecutionStats stats = emu.GetStats();
    EXPECT_GT(stats.total_instructions, 0u);
    EXPECT_GT(stats.alu_count, 0u);
    EXPECT_GT(stats.branch_count, 0u);
    EXPECT_GT(stats.jump_count, 0u);
}

// Тест 2: Инвариант регистра zero (x0) — запись в него должна игнорироваться
TEST(EmulatorTest, ZeroRegisterInvariant) {
    // Инструкция addi x0, x0, 42 пытается записать число в x0
    const std::vector<uint32_t> program = {
        0x02A00013
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Step();

    EXPECT_EQ(emu.GetRegister(0), 0u);
}

TEST(EmulatorTest, ZeroRegisterInvariant_ViaSetRegister) {
    Emulator emu;
    emu.SetRegister(0, 0xCAFEBABE);
    EXPECT_EQ(emu.GetRegister(0), 0u);
}

// Тест 3: Проверка условных переходов BLT, BGE, BEQ

// BLT: переход если rs1 < rs2
TEST(EmulatorTest, BranchBLT_Taken) {
    const std::vector<uint32_t> program = {
        0x00100513,  // addi x10, x0, 1
        0x00500593,  // addi x11, x0, 5
        0x00B54463,  // blt  x10, x11, 8 -> пропуск следующей инструкции
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 0u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

TEST(EmulatorTest, BranchBLT_NotTaken) {
    const std::vector<uint32_t> program = {
        0x00500513,  // addi x10, x0, 5
        0x00100593,  // addi x11, x0, 1
        0x00B54463,  // blt  x10, x11, 8 (не срабатывает)
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 99u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

// BGE: переход если rs1 >= rs2
TEST(EmulatorTest, BranchBGE_Taken) {
    const std::vector<uint32_t> program = {
        0x00500513,  // addi x10, x0, 5
        0x00500593,  // addi x11, x0, 5
        0x00B55463,  // bge  x10, x11, 8 (5 >= 5 истинно)
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 0u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

// BEQ: переход если rs1 == rs2
TEST(EmulatorTest, BranchBEQ_Taken) {
    const std::vector<uint32_t> program = {
        0x00300513,  // addi x10, x0, 3
        0x00300593,  // addi x11, x0, 3
        0x00B50463,  // beq  x10, x11, 8 (3 == 3 истинно)
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 0u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

TEST(EmulatorTest, BranchBEQ_NotTaken) {
    const std::vector<uint32_t> program = {
        0x00300513,  // addi x10, x0, 3
        0x00400593,  // addi x11, x0, 4
        0x00B50463,  // beq  x10, x11, 8 (3 == 4 ложно)
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 99u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

// Тест 4: Чтение и запись в память (LW / SW)
TEST(EmulatorTest, LoadStoreRoundTrip) {
    // Сохраняем 42 по адресу 256, затем считываем обратно в x12
    const std::vector<uint32_t> program = {
        0x02A00513,  // addi x10, x0, 42
        0x10000593,  // addi x11, x0, 256
        0x00A5A023,  // sw   x10, 0(x11)
        0x0005A603   // lw   x12, 0(x11)
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 42u);
}

// Тест 5: Вычисление 10-го числа Фибоначчи (fib(10) = 55)
TEST(EmulatorTest, Fibonacci10) {
    const std::vector<uint32_t> program = {
        0x00A00513,  // addi x10, x0, 10
        0x00000593,  // addi x11, x0, 0
        0x00100613,  // addi x12, x0, 1
        0x00000693,  // addi x13, x0, 0
        0x00A68C63,  // beq  x13, x10, 24
        0x00C58733,  // add  x14, x11, x12
        0x000605B3,  // add  x11, x12, x0
        0x00070633,  // add  x12, x14, x0
        0x00168693,  // addi x13, x13, 1
        0xFEDFF06F,  // jal  x0, -20
        0x0000006F   // jal  x0, 0
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(500);

    EXPECT_EQ(emu.GetRegister(11), 55u);
}
