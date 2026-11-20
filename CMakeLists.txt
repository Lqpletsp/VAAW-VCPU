cmake_minimum_required(VERSION 3.10)
project(VAAW_VCPU C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(
    .
    v_sys
    v_sys/v_cpu
    v_sys/v_mmu
)

add_executable(VAAW_VM_C
    main.c
    v_sys/v_sys.c
    v_sys/v_cpu/v_cpu.c
    v_sys/v_mmu/v_mmu.c
)