#include "v_sys/v_sys.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  // format: <executable name> <disk address>
  if (argc != 2) {
    printf("invalid argument format\n");
    return 1;
  }

  vm_t *vm_ptr = get_v_sys_ptr();
  if (vm_ptr == NULL) {
    printf("memory allocation for virtual machine failed\n");
    printf("stopping now\n");
    return 1;
  }
  if (!run_vm(vm_ptr, argv[1])) {
    printf("vm boot failed: most likely a secondary disk error\n");
    printf("stopping now\n");
  }
  return 0;
}
