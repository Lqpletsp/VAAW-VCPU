#include "v_mmu.h"
#include "../v_cpu/v_cpu.h"
#include <stdint.h>
static void change_current_process() {
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
  change_current_process();
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
  if (fseek(c_vm_disk, init_pos, SEEK_SET) != 0) {
    load_reg_for_err(file_access_failed);
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
  // add the process to queue
  add_new_process_in_queue(&new_process);
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
