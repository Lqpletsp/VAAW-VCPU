#define V_RAM_CAPACITY (64*1024)
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    // 32 bit system
    uint8_t container[V_RAM_CAPACITY]; 
    int v_ram_capacity; 
    // g_regs -> general registers signed
    uint32_t ipr, g_regu[5]; 
    // ipr -> instruction pointer register 
    uint8_t f_regu[3]; 
    // f_regu[0] indicates error signal, f_regu[1] indicates type of error,
    // f_regu[2] indicates the mode (0 is operate, 1 is idle)
    int p_regs[12]; 
    // p_regs -> process registers signed
} vm_t; 
extern vm_t* c_vm; 
vm_t* get_v_sys_ptr(); 
void run_vm(vm_t* vm_ptr); 