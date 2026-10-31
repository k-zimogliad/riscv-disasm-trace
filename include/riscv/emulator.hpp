#pragma once

#include "riscv/instruction.hpp"
#include "riscv/types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace riscv {

// Эмулятор процессора RV32I с плоской памятью 64 КБ
class Emulator {
public:
    explicit Emulator(size_t memory_size = 65536);

    void LoadProgram(const std::vector<uint32_t>& words, uint32_t start_address = 0);

    void Step();
    void Run(size_t max_steps = 10000);
    bool IsHalted() const noexcept { return halted_; }

    uint32_t GetRegister(size_t idx) const;
    void SetRegister(size_t idx, uint32_t val);

    uint32_t GetPC() const noexcept { return pc_; }
    void SetPC(uint32_t pc) noexcept { pc_ = pc; }

    std::string DumpRegisters() const;
    ExecutionStats GetStats() const noexcept { return stats_; }

private:
    std::array<uint32_t, 32> regs_{};
    uint32_t pc_ = 0;
    std::vector<uint8_t> memory_;
    ExecutionStats stats_{};
    bool halted_ = false;

    void Execute(const Instruction& inst);
    uint32_t ReadWord(uint32_t address) const;
    void WriteWord(uint32_t address, uint32_t value);
    void UpdateStats(const Instruction& inst);
};

} // namespace riscv
