#include "riscv/emulator.hpp"
#include "riscv/decoder.hpp"
#include "riscv/types.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace riscv {

Emulator::Emulator(size_t memory_size)
    : memory_(memory_size, 0) {
    regs_.fill(0);
}

void Emulator::LoadProgram(const std::vector<uint32_t>& words, uint32_t start_address) {
    for (size_t i = 0; i < words.size(); ++i) {
        uint32_t addr = start_address + static_cast<uint32_t>(i * 4);
        WriteWord(addr, words[i]);
    }
    pc_ = start_address;
    halted_ = false;
}

void Emulator::Step() {
    if (halted_) {
        return;
    }

    // Выборка инструкции по адресу PC
    uint32_t word = ReadWord(pc_);

    Instruction inst = Decoder::Decode(word);
    if (!inst.is_valid) {
        halted_ = true;
        return;
    }

    Execute(inst);

    // Регистр x0 всегда равен нулю в RISC-V
    regs_[0] = 0;

    UpdateStats(inst);
}

void Emulator::Run(size_t max_steps) {
    for (size_t i = 0; i < max_steps && !halted_; ++i) {
        Step();
    }
}

uint32_t Emulator::GetRegister(size_t idx) const {
    if (idx >= 32) {
        throw std::out_of_range("Некорректный индекс регистра (0-31)");
    }
    return regs_[idx];
}

void Emulator::SetRegister(size_t idx, uint32_t val) {
    if (idx >= 32) {
        throw std::out_of_range("Некорректный индекс регистра (0-31)");
    }
    // Запись в x0 игнорируется
    if (idx != 0) {
        regs_[idx] = val;
    }
}

std::string Emulator::DumpRegisters() const {
    std::ostringstream out;

    for (int i = 0; i < 32; ++i) {
        out << " x" << std::dec << std::setw(2) << std::setfill(' ') << i
            << " (" << std::setw(4) << std::setfill(' ') << ABI_REG_NAMES[static_cast<size_t>(i)] << ") = 0x"
            << std::hex << std::setw(8) << std::setfill('0') << regs_[static_cast<size_t>(i)]
            << "\n";
    }
    out << "  pc            = 0x"
        << std::hex << std::setw(8) << std::setfill('0') << pc_ << "\n";

    return out.str();
}

void Emulator::Execute(const Instruction& inst) {
    uint32_t next_pc = pc_ + 4;

    switch (inst.id) {
        // R-тип: арифметика и логика
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

        // I-тип: константы
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

        // U-тип
        case InstructionId::LUI:
            regs_[inst.rd] = static_cast<uint32_t>(inst.immediate);
            break;
        case InstructionId::AUIPC:
            regs_[inst.rd] = pc_ + static_cast<uint32_t>(inst.immediate);
            break;

        // Переходы
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

        // Ветвления
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

        // Память
        case InstructionId::LW: {
            uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            regs_[inst.rd] = ReadWord(addr);
            break;
        }
        case InstructionId::SW: {
            uint32_t addr = regs_[inst.rs1] + static_cast<uint32_t>(inst.immediate);
            WriteWord(addr, regs_[inst.rs2]);
            break;
        }

        default:
            halted_ = true;
            break;
    }

    pc_ = next_pc;
}

uint32_t Emulator::ReadWord(uint32_t address) const {
    if (static_cast<size_t>(address) + 3 >= memory_.size()) {
        return 0;
    }
    // Сборка 32-битного слова из 4 байт (Little Endian)
    return static_cast<uint32_t>(memory_[address])
         | (static_cast<uint32_t>(memory_[address + 1]) << 8)
         | (static_cast<uint32_t>(memory_[address + 2]) << 16)
         | (static_cast<uint32_t>(memory_[address + 3]) << 24);
}

void Emulator::WriteWord(uint32_t address, uint32_t value) {
    if (static_cast<size_t>(address) + 3 >= memory_.size()) {
        return;
    }
    // Запись в память по байтам (Little Endian)
    memory_[address]     = static_cast<uint8_t>(value & 0xFF);
    memory_[address + 1] = static_cast<uint8_t>((value >> 8)  & 0xFF);
    memory_[address + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    memory_[address + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void Emulator::UpdateStats(const Instruction& inst) {
    stats_.total_instructions++;

    switch (GetInstructionGroup(inst.id)) {
        case InstructionGroup::ALU:     stats_.alu_count++;    break;
        case InstructionGroup::Branch:  stats_.branch_count++; break;
        case InstructionGroup::Jump:    stats_.jump_count++;   break;
        case InstructionGroup::Memory:  stats_.memory_count++; break;
        case InstructionGroup::Unknown: break;
    }
}

} // namespace riscv
