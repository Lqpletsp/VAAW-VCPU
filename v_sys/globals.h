#ifndef GLOBALS_H
#define GLOBALS_H
#define V_RAM_CAPACITY (64 * 1024)
#define PROCESS_MAP_CAPACITY ((V_RAM_CAPACITY) / (4 * 1024))
#define P_REGS_QUANTITY (10)
#define V_DISK_CAPACITY (4 * 1024 * 1024) // 4 MB
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
  process *current_process;
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
  int capacity;
} ram;
typedef struct {
  FILE *container;
  int disk_capacity;
} disk;
typedef struct {
  // 32 bit system
  cpu v_cpu;
  ram v_ram;
  mmu v_mmu;
  disk v_disk;
} vm_t;
extern vm_t *c_vm;

inline uint8_t create_virtual_disk(char *filename) {
  if (c_vm == NULL)
    return 0;
  c_vm->v_disk.container = fopen(filename, "wb");
  if (c_vm->v_disk.container == NULL)
    return 0;
  uint8_t zero_buffer[8192] = {0};
  // 8kb of buffer
  for (size_t i = 0; i < V_DISK_CAPACITY; i += sizeof(zero_buffer)) {
    if (fwrite(zero_buffer, 1, sizeof(zero_buffer), c_vm->v_disk.container) !=
        sizeof(zero_buffer)) {
      fclose(c_vm->v_disk.container);
      return 0;
    }
  }
  return 1;
}
#endif
