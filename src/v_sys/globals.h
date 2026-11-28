#ifndef GLOBALS_H
#define GLOBALS_H
#define V_RAM_CAPACITY (64 * 1024)
#define PROCESS_MAP_CAPACITY ((V_RAM_CAPACITY) / (4 * 1024))
#define P_REGS_QUANTITY (10)
#include <stdint.h>
#include <stdio.h>

typedef struct {
  int front;
  int back;
} text_segment;
typedef struct {
  int front;
  int back;
} stack_segment;
typedef struct {
  int front;
  int back;
} data_segment;
typedef struct {
  int front;
  int back;
} heap_segment;
typedef struct {
  int pid;
  text_segment code;
  stack_segment stack;
  data_segment data;
  heap_segment heap;
  uint8_t state;
  // 0 means ready
  // 1 means halt
  // 2 means terminated
  // a process stays on the process_map until
  // it is overwritten if the current process.state is 2
} process;

typedef struct {
  process container[PROCESS_MAP_CAPACITY];
  int rear, front, capacity, size;
} process_queue;
typedef struct {
  int container[PROCESS_MAP_CAPACITY];
  int top, capacity, size;
} pid_stack;
typedef struct {
  process_queue process_map;
  pid_stack pid_stack;
  process *current_process, latest_process;
  uint32_t available_RAM_slot;
} mmu;
typedef struct {
  uint32_t ipr, g_regu[5];
  // ipr -> instruction pointer register
  uint8_t f_regu[3];
  // flag registers
  // f_regu[0] indicates error signal, f_regu[1] indicates type of error,
  // f_regu[2] indicates the mode (0 is operate, 1 is idle)
  uint32_t stack_pointer_register;
  int32_t p_regs[P_REGS_QUANTITY];
  int32_t p_regs_quantity;
  // process registers
  // p_regs[0] = result register
  // p_regs[1] = stack pointer
} cpu;
typedef struct {
  uint8_t container[V_RAM_CAPACITY];
  uint32_t capacity;
} ram;
typedef struct {
  FILE *container;
  uint64_t disk_capacity;
} disk;
typedef struct {
  // 32 bit cpu
  cpu v_cpu;
  ram v_ram;
  mmu v_mmu;
  disk v_disk;
} vm_t;

extern vm_t *c_vm;
#endif
