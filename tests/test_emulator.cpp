#include "riscv/emulator.hpp"
#include "riscv/decoder.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <vector>

using namespace riscv;

TEST(EmulatorTest, SumOneToFive) {
    // 1 + 2 + 3 + 4 + 5 = 15
    const std::vector<uint32_t> program = {
        0x00600513,  // addi x10, x0, 6
        0x00000593,  // addi x11, x0, 0
        0x00100613,  // addi x12, x0, 1
        0x00A65863,  // bge  x12, x10, 16 -> exit loop
        0x00C585B3,  // add  x11, x11, x12
        0x00160613,  // addi x12, x12, 1
        0xFF5FF06F,  // jal  x0, -12
        0x00100073   // ebreak
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(200);

    EXPECT_TRUE(emu.IsHalted());
    EXPECT_EQ(emu.GetRegister(11), 15u);
    EXPECT_EQ(emu.GetRegister(12), 6u);

    const ExecutionStats stats = emu.GetStats();
    EXPECT_GT(stats.total_instructions, 0u);
    EXPECT_GT(stats.alu_count, 0u);
    EXPECT_GT(stats.branch_count, 0u);
    EXPECT_GT(stats.jump_count, 0u);
    EXPECT_EQ(stats.system_count, 1u);
}

TEST(EmulatorTest, ZeroRegisterInvariant) {
    const std::vector<uint32_t> program = {
        0x02A00013,  // addi x0, x0, 42
        0x00100073   // ebreak
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Step();

    EXPECT_EQ(emu.GetRegister(0), 0u);
}

TEST(EmulatorTest, ZeroRegisterInvariant_ViaSetRegister) {
    Emulator emu;
    emu.SetRegister(0, 0x12345678);
    EXPECT_EQ(emu.GetRegister(0), 0u);
}

TEST(EmulatorTest, BranchBLT_Taken) {
    const std::vector<uint32_t> program = {
        0x00100513,  // addi x10, x0, 1
        0x00500593,  // addi x11, x0, 5
        0x00B54463,  // blt  x10, x11, 8
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
        0x00B54463,  // blt  x10, x11, 8
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 99u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

TEST(EmulatorTest, BranchBGE_Taken) {
    const std::vector<uint32_t> program = {
        0x00500513,  // addi x10, x0, 5
        0x00500593,  // addi x11, x0, 5
        0x00B55463,  // bge  x10, x11, 8
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 0u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

TEST(EmulatorTest, BranchBEQ_Taken) {
    const std::vector<uint32_t> program = {
        0x00300513,  // addi x10, x0, 3
        0x00300593,  // addi x11, x0, 3
        0x00B50463,  // beq  x10, x11, 8
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
        0x00B50463,  // beq  x10, x11, 8
        0x06300613,  // addi x12, x0, 99
        0x04D00693   // addi x13, x0, 77
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 99u);
    EXPECT_EQ(emu.GetRegister(13), 77u);
}

TEST(EmulatorTest, LoadStoreWordRoundTrip) {
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

TEST(EmulatorTest, ByteAndHalfwordMemory) {
    Emulator emu;

    // Direct memory API test
    emu.WriteByte(100, 0xFE);
    EXPECT_EQ(emu.ReadByte(100), 0xFE);

    emu.WriteHalf(200, 0x1234);
    EXPECT_EQ(emu.ReadHalf(200), 0x1234);

    // Assembly program: test LB (sign extension) and LBU (zero extension)
    // x11 = 100
    // lb  x12, 0(x11) -> should sign-extend 0xFE to 0xFFFFFFFE
    // lbu x13, 0(x11) -> should zero-extend 0xFE to 0x000000FE
    const std::vector<uint32_t> program = {
        0x06400593,  // addi x11, x0, 100
        0x00058603,  // lb   x12, 0(x11)
        0x0005C683   // lbu  x13, 0(x11)
    };

    emu.LoadProgram(program);
    emu.WriteByte(100, 0xFE);
    emu.Run(10);

    EXPECT_EQ(emu.GetRegister(12), 0xFFFFFFFEu);
    EXPECT_EQ(emu.GetRegister(13), 0x000000FEu);
}

TEST(EmulatorTest, EbreakHaltsExecution) {
    const std::vector<uint32_t> program = {
        0x00500513,  // addi x10, x0, 5
        0x00100073,  // ebreak
        0x00A00513   // addi x10, x0, 10 (should not be executed)
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run(10);

    EXPECT_TRUE(emu.IsHalted());
    EXPECT_EQ(emu.GetRegister(10), 5u);
}

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
        0x00100073   // ebreak
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.Run();

    EXPECT_TRUE(emu.IsHalted());
    EXPECT_EQ(emu.GetRegister(11), 55u);

    const ExecutionStats stats = emu.GetStats();
    EXPECT_EQ(stats.total_instructions, 66u);
    EXPECT_EQ(stats.alu_count, 44u);
    EXPECT_EQ(stats.branch_count, 11u);
    EXPECT_EQ(stats.jump_count, 10u);
    EXPECT_EQ(stats.memory_count, 0u);
    EXPECT_EQ(stats.system_count, 1u);
}

TEST(EmulatorTest, MemoryFault_OutOfBoundsRead) {
    Emulator emu(1024);

    // Try reading past 1024
    uint32_t val = emu.ReadWord(2048);
    EXPECT_EQ(val, 0u);
    EXPECT_TRUE(emu.IsHalted());
    EXPECT_TRUE(emu.HasMemoryFault());
    EXPECT_EQ(emu.GetTrapCause(), TrapCause::LoadAddressFault);
    EXPECT_EQ(emu.GetFaultAddress(), 2048u);
}

TEST(EmulatorTest, MemoryFault_OutOfBoundsWrite) {
    Emulator emu(512);

    emu.WriteByte(1000, 0xAA);
    EXPECT_TRUE(emu.IsHalted());
    EXPECT_TRUE(emu.HasMemoryFault());
    EXPECT_EQ(emu.GetTrapCause(), TrapCause::StoreAddressFault);
    EXPECT_EQ(emu.GetFaultAddress(), 1000u);
}

TEST(EmulatorTest, TracingExecution) {
    const std::vector<uint32_t> program = {
        0x00A00513,  // addi a0, zero, 10
        0x00550593,  // addi a1, a0, 5
        0x00100073   // ebreak
    };

    Emulator emu;
    emu.LoadProgram(program);
    emu.EnableTracing(true);
    emu.Run();

    EXPECT_TRUE(emu.IsHalted());
    EXPECT_EQ(emu.GetTrapCause(), TrapCause::Breakpoint);

    const auto& trace = emu.GetTraceLog();
    ASSERT_EQ(trace.size(), 3u);

    // Verify trace includes PC, instruction and register changes
    EXPECT_NE(trace[0].find("[PC: 0x00000000]"), std::string::npos);
    EXPECT_NE(trace[0].find("addi a0, zero, 10"), std::string::npos);
    EXPECT_NE(trace[0].find("a0 = 0x0000000a"), std::string::npos);

    EXPECT_NE(trace[1].find("[PC: 0x00000004]"), std::string::npos);
    EXPECT_NE(trace[1].find("addi a1, a0, 5"), std::string::npos);
    EXPECT_NE(trace[1].find("a1 = 0x0000000f"), std::string::npos);

    EXPECT_NE(trace[2].find("ebreak"), std::string::npos);
}

TEST(EmulatorTest, TracingCustomCallback) {
    const std::vector<uint32_t> program = {
        0x00100513,  // addi a0, zero, 1
        0x00100073   // ebreak
    };

    Emulator emu;
    emu.LoadProgram(program);

    std::vector<std::string> custom_lines;
    emu.SetTraceCallback([&](const std::string& line) {
        custom_lines.push_back(line);
    });
    emu.EnableTracing(true);
    emu.Run();

    ASSERT_EQ(custom_lines.size(), 2u);
    EXPECT_NE(custom_lines[0].find("addi a0, zero, 1"), std::string::npos);
}

TEST(EmulatorTest, LoadBinaryRoundTrip) {
    namespace fs = std::filesystem;
    const fs::path temp_bin = fs::temp_directory_path() / "test_prog.bin";

    // Write binary machine code
    const std::vector<uint32_t> original = {
        0x02A00513,  // addi a0, zero, 42
        0x00100073   // ebreak
    };

    {
        std::ofstream out(temp_bin, std::ios::binary);
        out.write(reinterpret_cast<const char*>(original.data()), original.size() * sizeof(uint32_t));
    }

    Emulator emu;
    emu.LoadBinary(temp_bin, 0);
    emu.Run();

    EXPECT_TRUE(emu.IsHalted());
    EXPECT_EQ(emu.GetRegister(10), 42u);
    EXPECT_EQ(emu.GetTrapCause(), TrapCause::Breakpoint);

    fs::remove(temp_bin);
}

TEST(EmulatorTest, LoadBinaryNonExistentThrows) {
    Emulator emu;
    EXPECT_THROW(emu.LoadBinary("non_existent_file_xyz.bin"), std::runtime_error);
}
