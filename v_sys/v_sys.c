#include "globals.h"
#include "stdlib.h"
#include "v_cpu/v_cpu.h"
vm_t *c_vm;
vm_t *get_v_sys_ptr() { return (vm_t *)malloc(sizeof(vm_t)); }
static uint8_t boot_sys(vm_t *vm_ptr) {
  if (vm_ptr == NULL)
    return 0;
  c_vm = vm_ptr;
  c_vm->v_ram.capacity = V_RAM_CAPACITY;
  c_vm->v_mmu.current_process = NULL;
  c_vm->v_cpu.f_regu[2] = 1;
  c_vm->v_cpu.p_regs_quantity = P_REGS_QUANTITY;
  c_vm->v_mmu.process_map.capacity = PROCESS_MAP_CAPACITY;
  c_vm->v_mmu.process_map.front = 0;
  c_vm->v_mmu.process_map.rear = 0;
  c_vm->v_mmu.process_map.size = 0;
  for (int pid = PROCESS_MAP_CAPACITY - 1; pid >= 0; --pid) {
    c_vm->v_mmu.pid_stack.container[pid] = pid;
  }
  c_vm->v_mmu.pid_stack.top = 0;
  c_vm->v_mmu.pid_stack.capacity = PROCESS_MAP_CAPACITY;
  c_vm->v_mmu.pid_stack.top = 0;
  c_vm->v_mmu.pid_stack.size = 0;
  c_vm->v_disk.container = NULL;
  // intialize v_disk
  char *v_disk_name = "VAAW_v_disk";
  c_vm->v_disk.container = fopen(v_disk_name, "rb");
  if (c_vm->v_disk.container == NULL) {
    uint8_t x = create_virtual_disk(v_disk_name);
    if (!x)
      return x;
  }
  c_vm->v_disk.disk_capacity = V_DISK_CAPACITY;
  return 1;
}

uint8_t run_vm(vm_t *vm_ptr) {
  if (!boot_sys(vm_ptr))
    return 0;
  sys_loop();
  return 1;
}
