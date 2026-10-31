#include "riscv/decoder.hpp"
#include "riscv/disassembler.hpp"
#include "riscv/emulator.hpp"
#include "riscv/types.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    // fib(10) на RV32I:
    // a0 (x10) = 10 (n)
    // a1 (x11) = 0  (fib(0))
    // a2 (x12) = 1  (fib(1))
    // a3 (x13) = 0  (счетчик i)
    // a4 (x14) = temp
    const std::vector<uint32_t> program = {
        0x00A00513,  // addi a0, zero, 10
        0x00000593,  // addi a1, zero, 0
        0x00100613,  // addi a2, zero, 1
        0x00000693,  // addi a3, zero, 0
        0x00A68C63,  // beq  a3, a0, 24
        0x00C58733,  // add  a4, a1, a2
        0x000605B3,  // add  a1, a2, zero
        0x00070633,  // add  a2, a4, zero
        0x00168693,  // addi a3, a3, 1
        0xFEDFF06F,  // jal  zero, -20
        0x00100073   // ebreak
    };

    std::cout << "--- Дизассемблирование программы (RV32I) ---\n";
    std::cout << "Адрес      Код команды  Ассемблер\n";
    std::cout << "--------------------------------------------\n";

    for (size_t i = 0; i < program.size(); ++i) {
        uint32_t addr = static_cast<uint32_t>(i * 4);
        riscv::Instruction inst = riscv::Decoder::Decode(program[i]);
        std::string asm_str = riscv::Disassembler::ToString(inst, true);

        std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr
                  << "     0x" << std::setw(8) << std::setfill('0') << program[i]
                  << "    " << asm_str << "\n";
    }

    std::cout << "\n--- Запуск эмулятора с пошаговой трассировкой (Trace) ---\n";
    riscv::Emulator emu;
    emu.LoadProgram(program);
    emu.EnableTracing(true, &std::cout);
    emu.Run();

    std::cout << "\nВыполнение завершено.";
    if (emu.HasMemoryFault()) {
        std::cout << " [ОШИБКА ДОСТУПА К ПАМЯТИ по адресу 0x" << std::hex << emu.GetFaultAddress() << "]\n";
    } else {
        std::cout << " Причина останова: "
                  << (emu.GetTrapCause() == riscv::TrapCause::Breakpoint ? "Breakpoint (EBREAK)" : "Halt")
                  << "\n";
    }
    std::cout << "fib(10) в регистре a1: " << std::dec << emu.GetRegister(11) << "\n\n";

    std::cout << "--- Дамп регистров ---\n";
    std::cout << emu.DumpRegisters(true);

    riscv::ExecutionStats stats = emu.GetStats();
    std::cout << "\n--- Статистика по инструкциям ---\n";
    std::cout << "Всего инструкций : " << std::dec << stats.total_instructions << "\n";
    std::cout << "  ALU            : " << stats.alu_count << "\n";
    std::cout << "  Ветвления      : " << stats.branch_count << "\n";
    std::cout << "  Переходы       : " << stats.jump_count << "\n";
    std::cout << "  Память         : " << stats.memory_count << "\n";
    std::cout << "  Системные      : " << stats.system_count << "\n";

    return 0;
}
