#include "v_mmu.h"
#include <stdint.h>

void change_current_process() {
  // [text][stack][heap][data] segments
  if (c_vm->v_mmu.process_map.front >= c_vm->v_mmu.process_map.capacity - 1)
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
