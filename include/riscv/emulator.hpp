#pragma once

#include "riscv/instruction.hpp"
#include "riscv/types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace riscv {

enum class TrapCause {
    None,
    InstructionAddressMisaligned,
    IllegalInstruction,
    Breakpoint,
    LoadAddressFault,
    StoreAddressFault,
    Ecall
};

class Emulator {
public:
    explicit Emulator(size_t memory_size = 65536);

    void LoadProgram(const std::vector<uint32_t>& words, uint32_t start_address = 0);
    void LoadBinary(const std::filesystem::path& path, uint32_t start_address = 0);

    void Step();
    void Run(size_t max_steps = 100000);
    bool IsHalted() const noexcept { return halted_; }

    TrapCause GetTrapCause() const noexcept { return trap_cause_; }
    bool HasMemoryFault() const noexcept {
        return trap_cause_ == TrapCause::LoadAddressFault || trap_cause_ == TrapCause::StoreAddressFault;
    }
    uint32_t GetFaultAddress() const noexcept { return fault_address_; }

    void EnableTracing(bool enable = true, std::ostream* os = nullptr);
    bool IsTracingEnabled() const noexcept { return tracing_enabled_; }
    void SetTraceCallback(std::function<void(const std::string& line)> callback);
    const std::vector<std::string>& GetTraceLog() const noexcept { return trace_log_; }

    uint32_t GetRegister(size_t idx) const;
    void SetRegister(size_t idx, uint32_t val);

    uint32_t GetPC() const noexcept { return pc_; }
    void SetPC(uint32_t pc) noexcept { pc_ = pc; }

    std::string DumpRegisters(bool non_zero_only = false) const;
    ExecutionStats GetStats() const noexcept { return stats_; }

    uint8_t ReadByte(uint32_t address) const;
    void WriteByte(uint32_t address, uint8_t value);

    uint16_t ReadHalf(uint32_t address) const;
    void WriteHalf(uint32_t address, uint16_t value);

    uint32_t ReadWord(uint32_t address) const;
    void WriteWord(uint32_t address, uint32_t value);

private:
    std::array<uint32_t, 32> regs_{};
    uint32_t pc_ = 0;
    std::vector<uint8_t> memory_;
    ExecutionStats stats_{};
    mutable TrapCause trap_cause_ = TrapCause::None;
    mutable uint32_t fault_address_ = 0;
    mutable bool halted_ = false;

    bool tracing_enabled_ = false;
    std::ostream* trace_stream_ = nullptr;
    std::function<void(const std::string&)> trace_callback_;
    std::vector<std::string> trace_log_;

    void Execute(const Instruction& inst);
    void UpdateStats(const Instruction& inst);
    void RecordTrace(uint32_t current_pc, const Instruction& inst, uint32_t old_rd_val);
};

} // namespace riscv
