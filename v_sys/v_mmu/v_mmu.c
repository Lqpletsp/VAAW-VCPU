#include "v_mmu.h"
#include "../v_cpu/v_cpu.h"
#include <stdint.h>

void change_current_process() {
  // [text][data][stack][heap] segments
  if (c_vm->v_mmu.process_map.front > c_vm->v_mmu.process_map.capacity - 1)
    c_vm->v_mmu.process_map.front = 0;
  else if (c_vm->v_mmu.process_map.size == 0) {
    // idle until new proecss in memory
    c_vm->v_cpu.f_regu[2] = 1;
    return;
  }
  c_vm->v_mmu.current_process =
      &c_vm->v_mmu.process_map.container[c_vm->v_mmu.process_map.front];
  ++c_vm->v_mmu.process_map.front;
  --c_vm->v_mmu.process_map.size;
  c_vm->v_cpu.ipr = c_vm->v_mmu.current_process->code.front;
}

static int get_top_pid_stack_data() {
  return c_vm->v_mmu.pid_stack.container[c_vm->v_mmu.pid_stack.top];
}
static uint8_t pop_top_pid_stack_data() {
  int top = c_vm->v_mmu.pid_stack.top;
  if (top == 0)
    return 0;
  --c_vm->v_mmu.pid_stack.top;
  --c_vm->v_mmu.pid_stack.size;
  return 1;
}
static uint8_t push_pid_in_pid_stack(int pid) {
  int top = c_vm->v_mmu.pid_stack.top;
  if (top > c_vm->v_mmu.pid_stack.capacity - 1)
    return 0;
  c_vm->v_mmu.pid_stack.container[top] = pid;
  ++c_vm->v_mmu.pid_stack.top;
  ++c_vm->v_mmu.pid_stack.size;
  return 1;
}
void terminate_current_process() {
  int pid = c_vm->v_mmu.current_process->pid;
  c_vm->v_mmu.process_map.container[pid].state = 2;
  push_pid_in_pid_stack(pid);
}
static uint8_t add_new_process_in_queue(process *np) {
  process_queue *p_map = &c_vm->v_mmu.process_map;
  if (p_map->size == p_map->capacity) {
    return 0;
  }
  p_map->container[np->pid] = *np;
  ++p_map->size;
  return 1;
}

void cprc_inst(uint32_t init_pos, uint32_t file_length) {
  // create process instruction
  if (c_vm->v_mmu.pid_stack.size == 0) {
    load_reg_for_err(pid_queue_full);
    return;
  }
  FILE *c_vm_disk = c_vm->v_disk.container;
  if (c_vm_disk == NULL) {
    load_reg_for_err(attempted_access_to_uninit_disk);
    return;
  }
  // after validation, need to create an empty process of 4kb
  uint32_t current_vacant_address = c_vm->v_mmu.available_RAM_slot;
  // giving each process 4KB of RAM
  if (current_vacant_address + 4095 > V_RAM_CAPACITY - 1) {
    load_reg_for_err(ran_out_of_memory);
    return;
  }
  int new_pid = get_top_pid_stack_data();
  pop_top_pid_stack_data();
  process new_process;
  new_process.pid = new_pid;
  int current_code_segment = current_vacant_address;
  new_process.code.front = current_vacant_address;
  current_vacant_address += 1023;
  new_process.code.back = current_vacant_address;
  new_process.data.front = ++current_vacant_address;
  current_vacant_address += 1023;
  new_process.data.back = current_vacant_address;
  new_process.stack.front = ++current_vacant_address;
  current_vacant_address += 1023;
  new_process.stack.back = current_vacant_address;
  new_process.heap.front = ++current_vacant_address;
  current_vacant_address += 1023;
  new_process.heap.back = current_vacant_address;
  c_vm->v_mmu.available_RAM_slot = ++current_vacant_address;
  // load the process
  if (fseek(c_vm_disk, init_pos, SEEK_SET) != 0) {
    load_reg_for_err(file_access_failed);
    return;
  }
  uint8_t io_buffer[1024];
  fread(io_buffer, 1, sizeof(io_buffer), c_vm_disk);
  int byte_i = 0, size_of_io_buffer = sizeof(io_buffer) / sizeof(io_buffer[0]);
  while (byte_i < size_of_io_buffer - 1) {
    if (current_code_segment > new_process.code.back) {
      load_reg_for_err(process_corruption);
      return;
    }
    c_vm->v_ram.container[current_code_segment] = io_buffer[byte_i];
    if ((enum isa)io_buffer[byte_i] == end)
      break;
    ++current_code_segment;
  }
  // add the process to queue
  add_new_process_in_queue(&new_process);
}

uint8_t load_first_128b_from_disk() {
  // create process instruction
  // special instruction for when a system begins
  if (c_vm->v_mmu.pid_stack.size != 0) {
    return 0;
  }
  FILE *c_vm_disk = c_vm->v_disk.container;
  if (c_vm_disk == NULL) {
    load_reg_for_err(attempted_access_to_uninit_disk);
    return 0;
  }
  // after validation, need to create an empty process of 4kb
  uint32_t current_vacant_address = c_vm->v_mmu.available_RAM_slot;
  // giving each process 4KB of RAM
  if (current_vacant_address + 23 > V_RAM_CAPACITY - 1) {
    load_reg_for_err(ran_out_of_memory);
    return 0;
  }
  int new_pid = get_top_pid_stack_data();
  pop_top_pid_stack_data();
  // 24 bytes given to the init process
  process new_process;
  new_process.pid = new_pid;
  int current_code_segment = 0;
  new_process.code.front = current_vacant_address;
  current_vacant_address += 15;
  new_process.code.back = current_vacant_address;
  new_process.data.front = ++current_vacant_address;
  current_vacant_address += 7;
  new_process.data.back = current_vacant_address;
  c_vm->v_mmu.available_RAM_slot = ++current_vacant_address;
  // load the process
  uint8_t init_io_buffer[128];
  fread(init_io_buffer, 1, sizeof(init_io_buffer), c_vm_disk);
  int byte_i = 0,
      size_of_io_buffer = sizeof(init_io_buffer) / sizeof(init_io_buffer[0]);
  while (byte_i < size_of_io_buffer - 1) {
    if (current_code_segment > new_process.code.back) {
      load_reg_for_err(process_corruption);
      return 0;
    }
    c_vm->v_ram.container[current_code_segment] = init_io_buffer[byte_i];
    if ((enum isa)init_io_buffer[byte_i] == end)
      break;
    ++current_code_segment;
  }
  // add the process to queue
  add_new_process_in_queue(&new_process);
  change_current_process();
  return 1;
}
uint8_t verify_memory_address(uint32_t address,
                              enum v_ram_segments segment_type) {
  switch (segment_type) {
  case text:
    return (address >= c_vm->v_mmu.current_process->code.front &&
            address <= c_vm->v_mmu.current_process->code.back)
               ? 1
               : 0;
  case data:
    return (address >= c_vm->v_mmu.current_process->data.front &&
            address <= c_vm->v_mmu.current_process->data.back)
               ? 1
               : 0;
  case heap:
    return (address >= c_vm->v_mmu.current_process->heap.front &&
            address <= c_vm->v_mmu.current_process->heap.back)
               ? 1
               : 0;
  case stack:
    return (address >= c_vm->v_mmu.current_process->stack.front &&
            address <= c_vm->v_mmu.current_process->stack.back)
               ? 1
               : 0;
  }
  return 0;
}
