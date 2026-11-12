#include "v_sys.h"
#include "v_cpu/v_cpu.h"
vm_t* get_v_sys_ptr(){return (vm_t*)malloc(sizeof(vm_t)); }

void run_vm(vm_t* vm_ptr){
    c_vm = vm_ptr; 
    c_vm->v_ram_capacity = sizeof(c_vm->container)/sizeof(c_vm->container[0]); 
    execute_instructions(); 
}
