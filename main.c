#include "v_sys/v_sys.h" 
#include <stdio.h>

int main(){
    vm_t *vm_ptr = get_v_sys_ptr(); 
    if (vm_ptr== NULL) {
        printf("Memory allocation for virtual machine failed\n"); 
        printf("Stopping now\n");
        return 1; 
    }
    run_vm(vm_ptr); 
    return 0; 
}