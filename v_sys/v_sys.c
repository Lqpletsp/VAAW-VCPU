#include "globals.h"
#include "stdlib.h"
#include "v_cpu/v_cpu.h"
#include <stdio.h>
vm_t *c_vm;
typedef struct {
  uint8_t result;
  uint64_t data;
} rtr_value;
rtr_value get_v_disk_size(FILE *fp) {
  if (fp == NULL)
    return (rtr_value){.result = 0, .data = 0};
  rtr_value rv;
  rv.data = fseek(fp, 0L, SEEK_END);
  rv.result = 1;
  return rv;
}
vm_t *get_v_sys_ptr() { return (vm_t *)malloc(sizeof(vm_t)); }
static uint8_t boot_sys(vm_t *vm_ptr, char *v_disk_address) {
  printf("booting_sys\n");
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
  printf("initializing virtual disk\n");
  c_vm->v_disk.container = fopen(v_disk_address, "rb+");
  if (c_vm->v_disk.container == NULL) {
    printf("no appropriate disk found\n");
    return 0;
  }
  printf("virtual disk found: %s\n", v_disk_address);
  rtr_value disk_metadata = get_v_disk_size(c_vm->v_disk.container);
  if (!disk_metadata.result) {
    printf("failed to retrieve metadata for disk\n");
    return 0;
  }
  printf("virutal disk size: %lu", disk_metadata.data);
  c_vm->v_disk.disk_capacity = disk_metadata.data;
  if (disk_metadata.data < 1024 * 1024) {
    printf("vritual disk size less than 1 mb cannot be used appropriately\n");
    return 0;
  }
  return 1;
}

uint8_t run_vm(vm_t *vm_ptr, char *disk_address) {
  if (!boot_sys(vm_ptr, disk_address)) {
    printf("boot failed\n");
    return 0;
  }
  printf("boot passed\n");
  printf("waking v_cpu\n");
  sys_loop();
  return 1;
}
