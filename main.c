#include "v_sys/v_sys.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  // format: <executable name> <directory location>
  vm_t *vm_ptr = get_v_sys_ptr();
  if (vm_ptr == NULL) {
    printf("Memory allocation for virtual machine failed\n");
    printf("Stopping now\n");
    return 1;
  }
  if (!run_vm(vm_ptr)) {
    printf("VM boot failed: most likely a secondary disk error");
  }
  return 0;
}
