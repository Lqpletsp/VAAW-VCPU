#include <stdint.h> 
#include "../v_sys.h"
enum isa : uint8_t {
  set, cipr, add_s, add_u,
  min_s, min_u, mlt_s, mlt_u,
  division_s, division_u,
  and, or, xor, not, end
}; 
enum v_cpu_idt : uint8_t {
  no_error, 
  divide_by_zero,
  invalid_ipr, 
  ipr_corruption, 
  invalid_memory_address,
  invalid_arithmetic_dest_code,
  invalid_dest_code,
  invalid_register, 
  unsigned_addition_overflow, 
  signed_addition_overflow, 
  unsigned_subtraction_overflow, 
  signed_subtraction_overflow, 
  unsigned_multiplication_overflow,
  signed_multiplication_overflow, 
  signed_division_overflow, 
  invalid_set_arguments, 
  invalid_cipr_code, 
  invalid_instruction, 
};
static uint8_t code_8b; 
vm_t* c_vm;
void load_reg_for_err(enum v_cpu_idt x){
  c_vm->f_regu[0] = 1; 
  c_vm->f_regu[1] = x; 
}
void increment_ipr(){
  uint32_t temp_ipr = c_vm->ipr; 
  if (++temp_ipr >= c_vm->v_ram_capacity-1){
    load_reg_for_err(ipr_corruption); 
    return; 
  }
  c_vm->ipr = temp_ipr; 
}
uint8_t get_1byte_uint(){ return (uint8_t)c_vm->container[c_vm->ipr]; }
uint32_t get_4byte_uint(){ 
  uint32_t temp_4byte_uint = (uint32_t)c_vm->container[c_vm->ipr]; 
  increment_ipr(); 
  if (c_vm->f_regu[0] == 1) return 0; 
  temp_4byte_uint |= (uint32_t)c_vm->container[c_vm->ipr] << 8; 
  increment_ipr(); 
  if (c_vm->f_regu[0] == 1) return 0; 
  temp_4byte_uint |= (uint32_t)c_vm->container[c_vm->ipr] << 16; 
  increment_ipr(); 
  if (c_vm->f_regu[0] == 1) return 0; 
  temp_4byte_uint |= (uint32_t)c_vm->container[c_vm->ipr] << 24; 
  return temp_4byte_uint; 
}
uint8_t get_1byte_uint_at_address(int address){
  if (address > c_vm->v_ram_capacity - 1){
    load_reg_for_err(invalid_memory_address); 
    return 0; 
  }
  return (uint8_t)(c_vm->container[address]); 
}
uint32_t get_4byte_uint_at_address(int address){
  if (address > c_vm->v_ram_capacity - 1 || address + 4 > c_vm->v_ram_capacity - 1){
    load_reg_for_err(invalid_memory_address); 
    return 0; 
  }
  uint32_t const_val = (uint32_t)c_vm->container[address]; 
  ++address; 
  const_val |= (uint32_t)c_vm->container[address] << 8; 
  ++address; 
  const_val |= (uint32_t)c_vm->container[address] << 16; 
  ++address; 
  const_val |= (uint32_t)c_vm->container[address] << 24; 
  return const_val; 
}
void v_alu(const uint32_t arg1, const uint32_t arg2, const uint8_t opr){
  int32_t arg1_s = (int32_t)arg1; 
  int32_t arg2_s = (int32_t)arg2; 
  // 0->unsigned addition, 1->signed addition, 2->unsigned subtraction,
  // 3->signed subtraction, 4->signed multiplication, 5->unsigned multiplication
  // 6->signed division, 7->unsigned division, 8->bitwise AND, 9->bitwise OR, 
  // 10->bitwise xor, 11->not
  switch(opr){
    case 0:{
      if (arg1 > UINT32_MAX - arg2){
        load_reg_for_err(unsigned_addition_overflow); 
        return; 
      }
      c_vm->p_regs[0] = arg1 + arg2;
      break;
    } 
    case 1:{
      if ((arg2_s > 0 && arg1_s > INT32_MAX - arg2_s) || (arg2_s < 0 && arg1_s < INT32_MIN - arg2_s)){
        load_reg_for_err(signed_addition_overflow); 
        return; 
      }
      c_vm->p_regs[0] = arg1_s + arg2_s; 
      break; 
    }
    case 2:{ 
      // a-b < INT32_MIN; a < INT32_MIN + b 
      if (arg2 > arg1){
        load_reg_for_err(unsigned_subtraction_overflow);
        return; 
      }
      c_vm->p_regs[0] = arg1 - arg2; 
      break;
    }
    case 3:{
      if ((arg2_s > 0 && arg1_s < INT32_MIN + arg2_s) || (arg2_s < 0 && arg1_s > INT32_MIN + arg2_s)){
        load_reg_for_err(signed_subtraction_overflow); 
        return; 
      }
      c_vm->p_regs[0] = arg1_s - arg2_s; 
      break; 
    }
    case 4: {
      uint64_t result_u = (uint64_t)arg1 * (uint64_t)arg2; 
      if (result_u > UINT32_MAX){
        load_reg_for_err(unsigned_multiplication_overflow);
        return; 
      }
      c_vm->p_regs[0] = (int32_t)result_u; 
      break; 
    }
    case 5: {
      int64_t result_s = (int64_t)arg1_s * (int64_t)arg2_s;
      if (result_s > INT32_MAX || result_s < INT32_MIN){
        load_reg_for_err(signed_multiplication_overflow); 
        return; 
      }
      c_vm->p_regs[0] = (int32_t)result_s; 
      break; 
    }
    case 6: {
      if (!arg2){
        load_reg_for_err(divide_by_zero); 
        return; 
      }
      c_vm->p_regs[0] = (int32_t)(arg1/arg2); 
      break; 
    }
    case 7: {
      if (!arg2){
        load_reg_for_err(divide_by_zero); 
        return; 
      }
      if (arg1_s == INT32_MIN && arg2_s == -1){
        load_reg_for_err(signed_division_overflow); 
        return; 
      }
      c_vm->p_regs[0] = (int32_t)(arg1_s/arg2_s); 
      break; 
    }
    case 8:{
      c_vm->p_regs[0] = arg1 & arg2; 
      break;
    }
    case 9: {
      c_vm->p_regs[0] = arg1 | arg2; 
      break; 
    }
    case 10: {
      c_vm->p_regs[0] = arg1 ^ arg2; 
      break;
    }
    case 11: {
      c_vm->p_regs[0] = ~arg1; 
      break; 
    }
  }
}
void set_inst(){
  // format: set <source> <destination>
  uint8_t dest_code; 
  uint32_t dest_address, data; 
  uint8_t memory_address = 0; 
  code_8b = get_1byte_uint(); 
  if (!code_8b){
    memory_address = 1; 
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t v_memory_address = (uint32_t)get_4byte_uint();  
    data = get_4byte_uint_at_address(v_memory_address); 
    if (c_vm->f_regu[0]) return;
  } else if (code_8b == 1){
    increment_ipr(); 
    code_8b = get_1byte_uint(); 
    if (!(code_8b > 0 && code_8b < sizeof(c_vm->p_regs)/sizeof(c_vm->p_regs[0]))){
      load_reg_for_err(invalid_register);
      return; 
    }
    if (c_vm->f_regu[0]) return; 
    data = (uint32_t)(c_vm->p_regs[code_8b - 1]); 
  } else if (code_8b == 2){
    data = get_4byte_uint();
    if (c_vm->f_regu[0]) return; 
  }else if(code_8b == 3){
    if (memory_address){
      load_reg_for_err(invalid_set_arguments); 
      return;; 
    }
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t addr = get_4byte_uint(); 
    if (addr > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return; 
    }
    data = get_4byte_uint_at_address(addr); 
  } else {
    load_reg_for_err(invalid_dest_code); 
    return; 
  }
  increment_ipr(); 
  if (c_vm->f_regu[0]) return; 
  code_8b = get_1byte_uint(); 
  // now pointing at src code

}
    
void load_reg_for_v_alu(){
  uint8_t memory_address = 0; 
  uint32_t arg1_32b, arg2_32b; 
  // dest code 0 is address
  // dest code 1 is registers
  // dest code 2 is constant value
  // dest code 3 is for data inside an address inside an address 
  // mov <memory address> <memory address> is an illegal command
  code_8b = get_1byte_uint(); 
  if (!code_8b){
    memory_address = 1; 
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t v_memory_address = (uint32_t)get_4byte_uint();  
    arg1_32b = get_4byte_uint_at_address(v_memory_address); 
    if (c_vm->f_regu[0]) return;
  } else if (code_8b == 1){
    increment_ipr(); 
    code_8b = get_1byte_uint(); 
    if (!(code_8b > 0 && code_8b < sizeof(c_vm->p_regs)/sizeof(c_vm->p_regs[0]))){
      load_reg_for_err(invalid_register);
      return; 
    }
    if (c_vm->f_regu[0]) return; 
    arg1_32b = (uint32_t)(c_vm->p_regs[code_8b - 1]); 
  } else if (code_8b == 2){
    arg1_32b = get_4byte_uint();
    if (c_vm->f_regu[0]) return; 
  } else if(code_8b == 3){
    memory_address = 1; 
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t addr = get_4byte_uint(); 
    if (addr > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return; 
    }
    arg1_32b = get_4byte_uint_at_address(addr); 
  } else {
    load_reg_for_err(invalid_dest_code); 
    return; 
  }
  increment_ipr(); 
  if (c_vm->f_regu[0]) return; 
  code_8b = get_1byte_uint(); 
  // now pointing at arg2
  if (!code_8b){
    memory_address = 1; 
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t v_memory_address = (uint32_t)get_4byte_uint();  
    arg2_32b = get_4byte_uint_at_address(v_memory_address); 
    if (c_vm->f_regu[0]) return;
  } else if (code_8b == 1){
    increment_ipr(); 
    code_8b = get_1byte_uint(); 
    if (!(code_8b > 0 && code_8b < sizeof(c_vm->p_regs)/sizeof(c_vm->p_regs[0]))){
      load_reg_for_err(invalid_register);
      return; 
    }
    if (c_vm->f_regu[0]) return; 
    arg2_32b = (uint32_t)(c_vm->p_regs[code_8b - 1]); 
  } else if (code_8b == 2){
    arg2_32b = get_4byte_uint();
    if (c_vm->f_regu[0]) return; 
  }else if(code_8b == 3){
    if (memory_address){
      load_reg_for_err(invalid_set_arguments); 
      return;; 
    }
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t addr = get_4byte_uint(); 
    if (addr > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return; 
    }
    arg2_32b = get_4byte_uint_at_address(addr); 
  } else {
    load_reg_for_err(invalid_dest_code); 
    return; 
  }
}
void add_u_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 0); 
}
void add_s_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 1); 
}
void min_u_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 2); 
}
void min_s_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 3); 
}
void mlt_u_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 4); 
}
void mlt_s_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 5); 
}
void division_u_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 6); 
}
void division_s_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 7); 
}
void and_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 8); 
}
void or_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 9); 
}
void xor_inst(){
  load_reg_for_v_alu(); 
  if (c_vm->f_regu[0] != 1) return; 
  v_alu(c_vm->g_regu[0], c_vm->g_regu[1], 10); 
}
void not_inst(){
  uint8_t code_8b = get_1byte_uint(); 
  uint32_t arg1_32b; 
  // dest code 0 is address
  // dest code 1 is registers
  // dest code 2 is constant value
  code_8b = get_1byte_uint(); 
  if (!code_8b){
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    arg1_32b = (uint32_t)get_4byte_uint();  
  } else if (code_8b > 0 && code_8b < sizeof(c_vm->p_regs)/sizeof(c_vm->p_regs[0])){
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    arg1_32b = (uint32_t)(c_vm->p_regs[code_8b - 1]); 
  } else if (code_8b == 2){
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    arg1_32b = get_4byte_uint(); 
  }else {
    load_reg_for_err(invalid_dest_code); 
    return; 
  }
  v_alu(arg1_32b, 0, 11); 
}
void cipr_inst(){
  // cipr -> change instruction pointer register
  // cipr <pointer_code> <pointer>
  // pointer_code 0 means to search the address in address
  // pointer_code 1 means registers
  // pointer_code 2 means relative indexing
  // pointer_code 3 means constant value
  code_8b = get_1byte_uint(); 
  uint32_t temp_cipr; 
  if (code_8b == 0){ 
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t addr = get_4byte_uint(); 
    temp_cipr = get_4byte_uint_at_address(addr);  
    if (temp_cipr > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return; 
    }
    c_vm->ipr = temp_cipr;
  } else if (code_8b == 1){
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint8_t reg_code = get_1byte_uint(); 
    if (!(reg_code > 0 && reg_code < sizeof(c_vm->p_regs)/sizeof(c_vm->p_regs[0]))){
      load_reg_for_err(invalid_register); 
      return; 
    }
    temp_cipr = c_vm->p_regs[reg_code - 1]; 
    if (temp_cipr > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return; 
    }
    c_vm->ipr = temp_cipr; 
  } else if (code_8b == 2){
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    uint32_t address_to_add = get_4byte_uint(); 
    if (c_vm->ipr + address_to_add > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return;
    }
    c_vm->ipr += address_to_add; 
  } else if (code_8b == 3){
    increment_ipr(); 
    if (c_vm->f_regu[0]) return; 
    temp_cipr = get_4byte_uint(); 
    if (temp_cipr > c_vm->v_ram_capacity - 1){
      load_reg_for_err(invalid_memory_address); 
      return; 
    }
    c_vm->ipr = temp_cipr; 
  } else {
    load_reg_for_err(invalid_cipr_code); 
    return; 
  }
}
void sys_loop(){
  void (*isa_methods[14])(void) = {
    set_inst, cipr_inst, add_s_inst, add_u_inst, 
    min_s_inst, min_u_inst, mlt_s_inst, mlt_u_inst, 
    division_s_inst, division_u_inst, and_inst, or_inst, 
    xor_inst, not_inst
  }; 
  uint8_t stall = 0, end = 0; 
  while (1){
    uint8_t current_inst = get_1byte_uint(); 
    uint32_t isa_size = sizeof(isa_methods)/sizeof(isa_methods[0]); 
    if ((uint32_t)current_inst <= isa_size - 1 && !c_vm->f_regu[3]){
      isa_methods[current_inst]();
    }
    if (c_vm->f_regu[0]) c_vm->f_regu[2] = 1; 
    else if (current_inst != isa_size + 1) {
      c_vm->f_regu[0] = 1; 
      c_vm->f_regu[1] = (uint8_t)invalid_instruction;  
      c_vm->f_regu[2] = 1; 
    } else {
      increment_ipr(); 
      --c_vm->ipr; 
      if (c_vm->f_regu[0]){
        c_vm->f_regu[0] = 0; 
        c_vm->f_regu[1] = 0; 
        c_vm->f_regu[2] = 1; 
      }
    }
  }
}
