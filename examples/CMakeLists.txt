add_executable(riscv_demo
    main.cpp
)

target_link_libraries(riscv_demo
    PRIVATE
        riscv_core
)
