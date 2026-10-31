#include "riscv/emulator.hpp"
#include "riscv/decoder.hpp"
#include "riscv/disassembler.hpp"
#include "riscv/types.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace riscv {

Emulator::Emulator(size_t memory_size)
    : memory_(memory_size, 0) {
    regs_.fill(0);
}

void Emulator::LoadProgram(const std::vector<uint32_t>& words, uint32_t start_address) {
    if (static_cast<size_t>(start_address) + words.size() * 4 > memory_.size()) {
        throw std::out_of_range("Program exceeds emulator memory bounds");
    }
    for (size_t i = 0; i < words.size(); ++i) {
        uint32_t addr = start_address + static_cast<uint32_t>(i * 4);
        WriteWord(addr, words[i]);
    }
    pc_ = start_address;
    halted_ = false;
    trap_cause_ = TrapCause::None;
    fault_address_ = 0;
}

void Emulator::LoadBinary(const std::filesystem::path& path, uint32_t start_address) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open binary file: " + path.string());
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0) {
        throw std::runtime_error("Error reading binary file: " + path.string());
    }

    if (static_cast<size_t>(start_address) + static_cast<size_t>(size) > memory_.size()) {
        throw std::out_of_range("Binary file exceeds emulator memory bounds");
    }

    if (size > 0) {
        file.read(reinterpret_cast<char*>(&memory_[start_address]), size);
        if (!file) {
            throw std::runtime_error("Failed to read complete binary file: " + path.string());
        }
    }

    pc_ = start_address;
    halted_ = false;
    trap_cause_ = TrapCause::None;
    fault_address_ = 0;
}

void Emulator::EnableTracing(bool enable, std::ostream* os) {
    tracing_enabled_ = enable;
    trace_stream_ = os;
}

void Emulator::SetTraceCallback(std::function<void(const std::string&)> callback) {
    trace_callback_ = std::move(callback);
}

void Emulator::Step() {
    if (halted_) {
        return;
    }

    const uint32_t current_pc = pc_;
    const uint32_t word = ReadWord(current_pc);
    if (halted_) {
        return;
    }

    const Instruction inst = Decoder::Decode(word);
    if (!inst.is_valid) {
        trap_cause_ = TrapCause::IllegalInstruction;
        fault_address_ = current_pc;
        halted_ = true;
        return;
    }

    const uint32_t old_rd_val = (inst.rd < 32) ? regs_[inst.rd] : 0;

    Execute(inst);
    regs_[0] = 0;
    UpdateStats(inst);

    if (tracing_enabled_) {
        RecordTrace(current_pc, inst, old_rd_val);
    }
}

void Emulator::RecordTrace(uint32_t current_pc, const Instruction& inst, uint32_t old_rd_val) {
    std::string disasm = Disassembler::ToString(inst, true);

    std::ostringstream line;
    line << "[PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << current_pc
         << "] 0x" << std::setw(8) << std::setfill('0') << inst.raw
         << "  " << std::left << std::setw(28) << std::setfill(' ') << disasm;

    if (inst.rd != 0 && (inst.type == InstructionType::R ||
                         inst.type == InstructionType::I ||
                         inst.type == InstructionType::U ||
                         inst.type == InstructionType::J)) {
        if (!HasMemoryFault()) {
            line << " | " << Disassembler::RegisterName(inst.rd, true)
                 << " = 0x" << std::right << std::hex << std::setw(8) << std::setfill('0') << regs_[inst.rd];
            if (regs_[inst.rd] != old_rd_val) {
                line << " (was 0x" << std::right << std::hex << std::setw(8) << std::setfill('0') << old_rd_val << ")";
            }
        }
    }

    std::string trace_str = line.str();
    trace_log_.push_back(trace_str);

    if (trace_stream_) {
        *trace_stream_ << trace_str << "\n";
    }
    if (trace_callback_) {
        trace_callback_(trace_str);
    }
}

void Emulator::Run(size_t max_steps) {
    for (size_t i = 0; i < max_steps && !halted_; ++i) {
        Step();
    }
}

uint32_t Emulator::GetRegister(size_t idx) const {
    if (idx >= 32) {
        throw std::out_of_range("Invalid register index (must be 0-31)");
    }
    return regs_[idx];
}

void Emulator::SetRegister(size_t idx, uint32_t val) {
    if (idx >= 32) {
        throw std::out_of_range("Invalid register index (must be 0-31)");
    }
    if (idx != 0) {
        regs_[idx] = val;
    }
}

std::string Emulator::DumpRegisters(bool non_zero_only) const {
    std::ostringstream out;

    for (size_t i = 0; i < 32; ++i) {
        if (non_zero_only && regs_[i] == 0 && i != 0) {
            continue;
        }
        out << " x" << std::dec << std::setw(2) << std::setfill(' ') << i
            << " (" << std::setw(4) << std::setfill(' ') << ABI_REG_NAMES[i] << ") = 0x"
            << std::hex << std::setw(8) << std::setfill('0') << regs_[i]
            << "\n";
    }
    out << "  pc            = 0x"
        << std::hex << std::setw(8) << std::setfill('0') << pc_ << "\n";

    return out.str();
}

void Emulator::Execute(const Instruction& inst) {
    uint32_t next_pc = pc_ + 4;

    switch (inst.id) {
        // R-type
        case InstructionId::ADD:
            regs_[inst.rd] = regs_[inst.rs1] + regs_[inst.rs2];
            break;
        case InstructionId::SUB:
            regs_[inst.rd] = regs_[inst.rs1] - regs_[inst.rs2];
            break;
        case InstructionId::SLL:
            regs_[inst.rd] = regs_[inst.rs1] << (regs_[inst.rs2] & 0x1F);
            break;
        case InstructionId::SLT:
            regs_[inst.rd] = (static_cast<int32_t>(regs_[inst.rs1]) < static_cast<int32_t>(regs_[inst.rs2])) ? 1 : 0;
            break;
        case InstructionId::SLTU:
            regs_[inst.rd] = (regs_[inst.rs1] < regs_[inst.rs2]) ? 1 : 0;
            break;
        case InstructionId::XOR:
            regs_[inst.rd] = regs_[inst.rs1] ^ regs_[inst.rs2];
            break;
        case InstructionId::SRL:
            regs_[inst.rd] = regs_[inst.rs1] >> (regs_[inst.rs2] & 0x1F);
            break;
        case InstructionId::SRA:
            regs_[inst.rd] = static_cast<uint32_t>(
                static_cast<int32_t>(regs_[inst.rs1]) >> static_cast<int>(regs_[inst.rs2] & 0x1F));
            break;
        case InstructionId::OR:
            regs_[inst.rd] = regs_[inst.rs1] | regs_[inst.rs2];
            break;
        case InstructionId::AND:
            regs_[inst.rd] = regs_[inst.rs1] & regs_[inst.rs2];
            break;

        // I-type ALU
        case InstructionId::ADDI:
            regs_[inst.rd] = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::SLTI:
            regs_[inst.rd] = (static_cast<int32_t>(regs_[inst.rs1]) < inst.immediate) ? 1 : 0;
            break;
        case InstructionId::SLTIU:
            regs_[inst.rd] = (regs_[inst.rs1] < static_cast<uint32_t>(inst.immediate)) ? 1 : 0;
            break;
        case InstructionId::XORI:
            regs_[inst.rd] = regs_[inst.rs1] ^ static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::ORI:
            regs_[inst.rd] = regs_[inst.rs1] | static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::ANDI:
            regs_[inst.rd] = regs_[inst.rs1] & static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::SLLI:
            regs_[inst.rd] = regs_[inst.rs1] << static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::SRLI:
            regs_[inst.rd] = regs_[inst.rs1] >> static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::SRAI:
            regs_[inst.rd] = static_cast<uint32_t>(
                static_cast<int32_t>(regs_[inst.rs1]) >> inst.immediate);
            break;

        // I-type Loads
        case InstructionId::LB: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            const int8_t val = static_cast<int8_t>(ReadByte(addr));
            regs_[inst.rd] = static_cast<uint32_t>(static_cast<int32_t>(val));
            break;
        }
        case InstructionId::LH: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            const int16_t val = static_cast<int16_t>(ReadHalf(addr));
            regs_[inst.rd] = static_cast<uint32_t>(static_cast<int32_t>(val));
            break;
        }
        case InstructionId::LW: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            regs_[inst.rd] = ReadWord(addr);
            break;
        }
        case InstructionId::LBU: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            regs_[inst.rd] = static_cast<uint32_t>(ReadByte(addr));
            break;
        }
        case InstructionId::LHU: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            regs_[inst.rd] = static_cast<uint32_t>(ReadHalf(addr));
            break;
        }

        // S-type Stores
        case InstructionId::SB: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            WriteByte(addr, static_cast<uint8_t>(regs_[inst.rs2] & 0xFF));
            break;
        }
        case InstructionId::SH: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            WriteHalf(addr, static_cast<uint16_t>(regs_[inst.rs2] & 0xFFFF));
            break;
        }
        case InstructionId::SW: {
            const uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            WriteWord(addr, regs_[inst.rs2]);
            break;
        }

        // U-type
        case InstructionId::LUI:
            regs_[inst.rd] = static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::AUIPC:
            regs_[inst.rd] = pc_ + static_cast<uint32_t>(inst.immediate);
            break;

        // Jumps
        case InstructionId::JAL:
            regs_[inst.rd] = pc_ + 4;
            next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::JALR: {
            uint32_t target = (regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate)) & ~1u;
            regs_[inst.rd] = pc_ + 4;
            next_pc = target;
            break;
        }

        // Branches
        case InstructionId::BEQ:
            if (regs_[inst.rs1] == regs_[inst.rs2]) {
                next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            }
            break;
        case InstructionId::BNE:
            if (regs_[inst.rs1] != regs_[inst.rs2]) {
                next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            }
            break;
        case InstructionId::BLT:
            if (static_cast<int32_t>(regs_[inst.rs1]) < static_cast<int32_t>(regs_[inst.rs2])) {
                next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            }
            break;
        case InstructionId::BGE:
            if (static_cast<int32_t>(regs_[inst.rs1]) >= static_cast<int32_t>(regs_[inst.rs2])) {
                next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            }
            break;
        case InstructionId::BLTU:
            if (regs_[inst.rs1] < regs_[inst.rs2]) {
                next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            }
            break;
        case InstructionId::BGEU:
            if (regs_[inst.rs1] >= regs_[inst.rs2]) {
                next_pc = pc_ + static_cast<uint32_t>(inst.immediate);
            }
            break;

        // System & Sync
        case InstructionId::ECALL:
            trap_cause_ = TrapCause::Ecall;
            halted_ = true;
            break;
        case InstructionId::EBREAK:
            trap_cause_ = TrapCause::Breakpoint;
            halted_ = true;
            break;

        case InstructionId::FENCE:
            // NOP in single-core emulator
            break;

        default:
            trap_cause_ = TrapCause::IllegalInstruction;
            halted_ = true;
            break;
    }

    if (next_pc == pc_) {
        halted_ = true;
    }

    pc_ = next_pc;
}

uint8_t Emulator::ReadByte(uint32_t address) const {
    if (address >= memory_.size()) {
        trap_cause_ = TrapCause::LoadAddressFault;
        fault_address_ = address;
        halted_ = true;
        return 0;
    }
    return memory_[address];
}

void Emulator::WriteByte(uint32_t address, uint8_t value) {
    if (address >= memory_.size()) {
        trap_cause_ = TrapCause::StoreAddressFault;
        fault_address_ = address;
        halted_ = true;
        return;
    }
    memory_[address] = value;
}

uint16_t Emulator::ReadHalf(uint32_t address) const {
    if (static_cast<size_t>(address) + 1 >= memory_.size()) {
        trap_cause_ = TrapCause::LoadAddressFault;
        fault_address_ = address;
        halted_ = true;
        return 0;
    }
    return static_cast<uint16_t>(memory_[address])
         | static_cast<uint16_t>(static_cast<uint16_t>(memory_[address + 1]) << 8);
}

void Emulator::WriteHalf(uint32_t address, uint16_t value) {
    if (static_cast<size_t>(address) + 1 >= memory_.size()) {
        trap_cause_ = TrapCause::StoreAddressFault;
        fault_address_ = address;
        halted_ = true;
        return;
    }
    memory_[address]     = static_cast<uint8_t>(value & 0xFF);
    memory_[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

uint32_t Emulator::ReadWord(uint32_t address) const {
    if (static_cast<size_t>(address) + 3 >= memory_.size()) {
        trap_cause_ = TrapCause::LoadAddressFault;
        fault_address_ = address;
        halted_ = true;
        return 0;
    }
    return static_cast<uint32_t>(memory_[address])
         | (static_cast<uint32_t>(memory_[address + 1]) << 8)
         | (static_cast<uint32_t>(memory_[address + 2]) << 16)
         | (static_cast<uint32_t>(memory_[address + 3]) << 24);
}

void Emulator::WriteWord(uint32_t address, uint32_t value) {
    if (static_cast<size_t>(address) + 3 >= memory_.size()) {
        trap_cause_ = TrapCause::StoreAddressFault;
        fault_address_ = address;
        halted_ = true;
        return;
    }
    memory_[address]     = static_cast<uint8_t>(value & 0xFF);
    memory_[address + 1] = static_cast<uint8_t>((value >> 8)  & 0xFF);
    memory_[address + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    memory_[address + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void Emulator::UpdateStats(const Instruction& inst) {
    stats_.total_instructions++;

    switch (GetInstructionGroup(inst.id)) {
        case InstructionGroup::ALU:    stats_.alu_count++;    break;
        case InstructionGroup::Branch: stats_.branch_count++; break;
        case InstructionGroup::Jump:   stats_.jump_count++;   break;
        case InstructionGroup::Memory: stats_.memory_count++; break;
        case InstructionGroup::System: stats_.system_count++; break;
        case InstructionGroup::Unknown: break;
    }
}

} // namespace riscv
