#include "v_cpu.h"
#include "../globals.h"
#include "../v_mmu/v_mmu.h"
#include <stdatomic.h>
#include <stdint.h>
static uint8_t code_8b;
extern vm_t *c_vm;
void load_reg_for_err(enum shared_idt x) {
  c_vm->v_cpu.f_regu[0] = 1;
  c_vm->v_cpu.f_regu[1] = x;
}
void increment_ipr() {
  uint32_t temp_ipr = ++c_vm->v_cpu.ipr;
  if (!verify_memory_address(temp_ipr, text)) {
    load_reg_for_err(ipr_corruption);
    return;
  }
  c_vm->v_cpu.ipr = temp_ipr;
}
uint8_t get_8b_uint() {
  uint8_t data = c_vm->v_ram.container[c_vm->v_cpu.ipr];
  increment_ipr();
  return data;
}
uint32_t get_32b_uint() {
  if (!verify_memory_address(c_vm->v_cpu.ipr + 4, text)) {
    load_reg_for_err(segmentation_violation);
    return 0;
  }
  uint32_t temp_4byte_uint = (uint32_t)c_vm->v_ram.container[c_vm->v_cpu.ipr];
  increment_ipr();
  temp_4byte_uint |= (uint32_t)c_vm->v_ram.container[c_vm->v_cpu.ipr] << 8;
  increment_ipr();
  temp_4byte_uint |= (uint32_t)c_vm->v_ram.container[c_vm->v_cpu.ipr] << 16;
  increment_ipr();
  temp_4byte_uint |= (uint32_t)c_vm->v_ram.container[c_vm->v_cpu.ipr] << 24;
  increment_ipr();
  return temp_4byte_uint;
}
uint16_t get_16b_uint() {
  uint16_t temp_16b_uint = (uint16_t)c_vm->v_ram.container[c_vm->v_cpu.ipr];
  increment_ipr();
  if (c_vm->v_cpu.f_regu[0])
    return 0;
  temp_16b_uint |= (uint16_t)c_vm->v_ram.container[c_vm->v_cpu.ipr] << 8;
  increment_ipr();
  return temp_16b_uint;
}
uint16_t get_16b_uint_at_address(int address) {
  if (address + 1 > c_vm->v_ram.capacity) {
    load_reg_for_err(invalid_memory_address);
    return 0;
  }
  uint16_t const_val = (uint16_t)c_vm->v_ram.container[address];
  ++address;
  const_val |= (uint16_t)c_vm->v_ram.container[address] << 8;
  return const_val;
}
uint8_t get_8b_uint_at_address(int address) {
  if (address > c_vm->v_ram.capacity - 1) {
    load_reg_for_err(invalid_memory_address);
    return 0;
  }
  return (uint8_t)(c_vm->v_ram.container[address]);
}
uint32_t get_32b_uint_at_address(int address) {
  if (!verify_memory_address(address + 3, data)) {
    load_reg_for_err(invalid_memory_address);
    return 0;
  }
  uint32_t const_val = (uint32_t)c_vm->v_ram.container[address];
  ++address;
  const_val |= (uint32_t)c_vm->v_ram.container[address] << 8;
  ++address;
  const_val |= (uint32_t)c_vm->v_ram.container[address] << 16;
  ++address;
  const_val |= (uint32_t)c_vm->v_ram.container[address] << 24;
  return const_val;
}
void v_alu(const uint32_t arg1, const uint32_t arg2, const uint8_t opr) {
  int32_t arg1_s = (int32_t)arg1;
  int32_t arg2_s = (int32_t)arg2;
  // 0->unsigned addition, 1->signed addition, 2->unsigned subtraction,
  // 3->signed subtraction, 4->signed multiplication, 5->unsigned multiplication
  // 6->signed division, 7->unsigned division, 8->bitwise AND, 9->bitwise OR,
  // 10->bitwise xor, 11->not
  switch (opr) {
  case 0: {
    if (arg1 > UINT32_MAX - arg2) {
      load_reg_for_err(unsigned_addition_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = arg1 + arg2;
    break;
  }
  case 1: {
    if ((arg2_s > 0 && arg1_s > INT32_MAX - arg2_s) ||
        (arg2_s < 0 && arg1_s < INT32_MIN - arg2_s)) {
      load_reg_for_err(signed_addition_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = arg1_s + arg2_s;
    break;
  }
  case 2: {
    if (arg2 > arg1) {
      load_reg_for_err(unsigned_subtraction_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = arg1 - arg2;
    break;
  }
  case 3: {
    if ((arg2_s > 0 && arg1_s < INT32_MIN + arg2_s) ||
        (arg2_s < 0 && arg1_s > INT32_MIN + arg2_s)) {
      load_reg_for_err(signed_subtraction_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = arg1_s - arg2_s;
    break;
  }
  case 4: {
    uint64_t result_u = (uint64_t)arg1 * (uint64_t)arg2;
    if (result_u > UINT32_MAX) {
      load_reg_for_err(unsigned_multiplication_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = (int32_t)result_u;
    break;
  }
  case 5: {
    int64_t result_s = (int64_t)arg1_s * (int64_t)arg2_s;
    if (result_s > INT32_MAX || result_s < INT32_MIN) {
      load_reg_for_err(signed_multiplication_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = (int32_t)result_s;
    break;
  }
  case 6: {
    if (!arg2) {
      load_reg_for_err(divide_by_zero);
      return;
    }
    c_vm->v_cpu.p_regs[0] = (int32_t)(arg1 / arg2);
    break;
  }
  case 7: {
    if (!arg2) {
      load_reg_for_err(divide_by_zero);
      return;
    }
    if (arg1_s == INT32_MIN && arg2_s == -1) {
      load_reg_for_err(signed_division_overflow);
      return;
    }
    c_vm->v_cpu.p_regs[0] = (int32_t)(arg1_s / arg2_s);
    break;
  }
  case 8: {
    c_vm->v_cpu.p_regs[0] = arg1 & arg2;
    break;
  }
  case 9: {
    c_vm->v_cpu.p_regs[0] = arg1 | arg2;
    break;
  }
  case 10: {
    c_vm->v_cpu.p_regs[0] = arg1 ^ arg2;
    break;
  }
  case 11: {
    c_vm->v_cpu.p_regs[0] = ~arg1;
    break;
  }
  }
}
typedef struct {
  uint32_t arg1, arg2;
} int_data_pair;

int_data_pair get_args_for_v_alu() {
  // arg code 0 means address
  // arg code 1 means registers
  // arg code 2 means constant
  uint8_t ram_operation = 0;
  code_8b = get_8b_uint();
  int_data_pair rtr_val;
  switch (code_8b) {
  case 0: {
    ram_operation = 1;
    uint32_t source_address = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return (int_data_pair){};
    if (!verify_memory_address(source_address, data)) {
      load_reg_for_err(segmentation_violation);
      return (int_data_pair){};
    }
    rtr_val.arg1 = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return (int_data_pair){};
    break;
  }
  case 1: {
    uint8_t register_index = get_8b_uint();
    if (register_index > P_REGS_QUANTITY - 1) {
      load_reg_for_err(invalid_register);
      return (int_data_pair){};
    }
    rtr_val.arg1 = c_vm->v_cpu.p_regs[register_index];
    break;
  }
  case 2: {
    rtr_val.arg1 = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return (int_data_pair){};
    break;
  }
  default:
    load_reg_for_err(invalid_operator_argument);
    break;
  }
  // now pointing at argument2 code
  code_8b = get_8b_uint();
  switch (code_8b) {
  case 0: {
    if (!ram_operation) {
      load_reg_for_err(illegal_argument_combination);
      return (int_data_pair){};
    }
    uint32_t source_address = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return (int_data_pair){};
    if (!verify_memory_address(source_address, data)) {
      load_reg_for_err(segmentation_violation);
      return (int_data_pair){};
    }
    rtr_val.arg2 = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return (int_data_pair){};
    break;
  }
  case 1: {
    uint8_t register_index = get_8b_uint();
    if (register_index > P_REGS_QUANTITY - 1) {
      load_reg_for_err(invalid_register);
      return (int_data_pair){};
    }
    rtr_val.arg2 = c_vm->v_cpu.p_regs[register_index];
    break;
  }
  case 2: {
    rtr_val.arg2 = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return (int_data_pair){};
    break;
  }
  default:
    load_reg_for_err(invalid_operator_argument);
    break;
  }
  return rtr_val;
}

// 0->unsigned addition, 1->signed addition, 2->unsigned subtraction,
// 3->signed subtraction, 4->signed multiplication, 5->unsigned multiplication
// 6->signed division, 7->unsigned division, 8->bitwise AND, 9->bitwise OR,
// 10->bitwise xor, 11->not
void add_u_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 0);
}
void add_s_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 1);
}
void min_u_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 2);
}
void min_s_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 3);
}
void mlt_u_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 4);
}
void mlt_s_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 5);
}
void div_u_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 6);
}
void div_s_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 7);
}
void and_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 8);
}
void or_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 9);
}
void xor_inst() {
  int_data_pair args_u = get_args_for_v_alu();
  v_alu(args_u.arg1, args_u.arg2, 10);
}
void not_inst() {
  // arg code 0 means address
  // arg code 1 means registers
  // arg code 2 means constant
  uint8_t ram_operation = 0;
  code_8b = get_8b_uint();
  uint32_t arg;
  switch (code_8b) {
  case 0: {
    uint32_t source_address = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return;
    if (!verify_memory_address(source_address, data)) {
      load_reg_for_err(segmentation_violation);
      return;
    }
    arg = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return;
    break;
  }
  case 1: {
    uint8_t register_index = get_8b_uint();
    if (register_index > P_REGS_QUANTITY - 1) {
      load_reg_for_err(invalid_register);
      return;
    }
    arg = c_vm->v_cpu.p_regs[register_index];
    break;
  }
  case 2: {
    arg = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return;
    break;
  }
  default:
    load_reg_for_err(invalid_operator_argument);
    break;
  }
  v_alu(arg, 0, 11);
}
uint32_t pop_and_get_data_in_stack() {
  uint32_t stack_ptr = c_vm->v_cpu.stack_pointer_register;
  if (!verify_memory_address(stack_ptr - 4, stack)) {
    load_reg_for_err(segmentation_violation);
    return 0;
  }
  uint8_t data_msb, data_lsb, data_ssb, data_slsb;
  uint32_t data_32b;
  // stack pointing at the most significant byte of the 32 bits
  data_msb = c_vm->v_ram.container[--stack_ptr];
  c_vm->v_ram.container[stack_ptr] = 0;
  data_ssb = c_vm->v_ram.container[--stack_ptr];
  c_vm->v_ram.container[stack_ptr] = 0;
  data_slsb = c_vm->v_ram.container[--stack_ptr];
  c_vm->v_ram.container[stack_ptr] = 0;
  data_lsb = c_vm->v_ram.container[--stack_ptr];
  c_vm->v_ram.container[stack_ptr] = 0;

  data_32b = (uint32_t)data_lsb;
  data_32b |= ((uint32_t)data_slsb) << 8;
  data_32b |= ((uint32_t)data_ssb) << 16;
  data_32b |= ((uint32_t)data_msb) << 24;

  c_vm->v_cpu.stack_pointer_register = stack_ptr;
  return data_32b;
}
void push_data_in_stack(uint32_t data) {
  if (!verify_memory_address(c_vm->v_cpu.stack_pointer_register, stack)) {
    load_reg_for_err(segmentation_violation);
    return;
  }
  uint8_t data_lsb, data_msb, data_ssb, data_slsb;
  // data_lsb -> least significant byte
  // data_msb -> most significant byte
  // data_ssb -> second significant byte
  // data_slsb -> second least significant byte
  data_lsb = (uint8_t)(data & 255);
  data_slsb = (uint8_t)((data >> 8) & 255);
  data_ssb = (uint8_t)((data >> 16) & 255);
  data_msb = (uint8_t)((data >> 24) & 255);
  uint32_t stack_ptr = c_vm->v_cpu.stack_pointer_register;
  // little endian format
  c_vm->v_ram.container[stack_ptr] = data_lsb;
  c_vm->v_ram.container[++stack_ptr] = data_slsb;
  c_vm->v_ram.container[++stack_ptr] = data_ssb;
  c_vm->v_ram.container[++stack_ptr] = data_msb;
  ++stack_ptr;
  c_vm->v_cpu.stack_pointer_register = stack_ptr;
}
void ret_inst() {
  // ret
  // ret command pops 32 bits from the stack
  uint32_t new_ipr = pop_and_get_data_in_stack();
  if (c_vm->v_cpu.f_regu[0])
    return;
  if (!verify_memory_address(new_ipr, text)) {
    load_reg_for_err(segmentation_violation);
    return;
  }
  c_vm->v_cpu.ipr = new_ipr - 1;
}
void call_inst() {
  // call +|- <constant>
  // call command pushes 32 bits to the stack
  int32_t change_in_ipr = get_32b_uint();
  push_data_in_stack(c_vm->v_cpu.ipr - 1);
  uint32_t new_ipr = c_vm->v_cpu.ipr + change_in_ipr;
  if (!verify_memory_address(new_ipr, text)) {
    load_reg_for_err(invalid_memory_address);
    return;
  }
  c_vm->v_cpu.ipr += (--change_in_ipr);
}
void cipr_inst() {
  // cipr -> change instruction pointer register
  // cipr <pointer_code> <pointer>
  // currently pointing at <pointer code>
  // pointer_code 0 means registers
  // pointer_code 1 means constant value
  code_8b = get_8b_uint();
  switch (code_8b) {
  case 0: {
    uint8_t reg_index = get_8b_uint();
    uint32_t new_cipr = c_vm->v_cpu.p_regs[reg_index];
    if (c_vm->v_cpu.f_regu[0])
      return;
    if (verify_memory_address(new_cipr, text)) {
      c_vm->v_cpu.f_regu[0] = 1;
      c_vm->v_cpu.f_regu[1] = invalid_memory_address;
      return;
    }
    c_vm->v_cpu.ipr = new_cipr;
    break;
  }
  case 1: {
    uint32_t new_cipr = get_32b_uint();
    if (!verify_memory_address(new_cipr, text)) {
      load_reg_for_err(invalid_memory_address);
      return;
    }
    c_vm->v_cpu.ipr = new_cipr;
    break;
  }
  default:
    c_vm->v_cpu.f_regu[0] = 1;
    c_vm->v_cpu.f_regu[1] = invalid_cipr_code;
  }
}
void relative_cipr_inst() {
  // cipr -> change instruction pointer register
  // cipr <relative pointer_code> <relative pointer>
  // currently pointing at <pointer code>
  // pointer_code 0 means registers
  // pointer_code 1 means constant value
  code_8b = get_8b_uint();
  switch (code_8b) {
  case 0: {
    uint8_t reg_index = get_8b_uint();
    int32_t change_in_cipr = c_vm->v_cpu.p_regs[reg_index];
    uint32_t new_cipr = c_vm->v_cpu.ipr + change_in_cipr;
    if (c_vm->v_cpu.f_regu[0])
      return;
    if (!verify_memory_address(new_cipr, text)) {
      load_reg_for_err(invalid_memory_address);
      return;
    }
    c_vm->v_cpu.ipr = new_cipr;
    break;
  }
  case 1: {
    int32_t change_in_cipr = get_32b_uint();
    uint32_t new_cipr = c_vm->v_cpu.ipr + change_in_cipr;
    if (!verify_memory_address(new_cipr, text)) {
      load_reg_for_err(invalid_memory_address);
      return;
    }
    c_vm->v_cpu.ipr = new_cipr;
    break;
  }
  default:
    load_reg_for_err(invalid_cipr_code);
    return;
  }
}

void set_32b_inst() {
  // set_32b <source code> <source> <desitnation code> <destination>
  // currently pointing at <source code>
  // source code 0 is address
  // source code 1 is p_regs
  // source code 2 is constant value
  // set32_b <address> <address> is an invalid instruction
  code_8b = get_8b_uint();
  uint32_t data;
  uint8_t ram_operation = 0;
  switch (code_8b) {
  case 0: {
    ram_operation = 1;
    increment_ipr();
    if (c_vm->v_cpu.f_regu[0])
      return;
    uint32_t source_address = get_32b_uint();
    data = get_32b_uint_at_address(source_address);
    if (c_vm->v_cpu.f_regu[0])
      return;
    break;
  }
  case 1: {
    increment_ipr();
    if (c_vm->v_cpu.f_regu[0])
      return;
    uint8_t register_index = get_8b_uint();
    if (register_index > P_REGS_QUANTITY - 1) {
      load_reg_for_err(invalid_register);
      return;
      data = c_vm->v_cpu.p_regs[register_index];
      break;
    }
  }
  case 2:
    increment_ipr();
    if (c_vm->v_cpu.f_regu[0])
      return;
    data = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return;
  }
  // pointing at <destination code>
  code_8b = get_8b_uint();
  if (c_vm->v_cpu.f_regu[0])
    return;
  switch (code_8b) {
  case 0: {
    if (ram_operation) {
      load_reg_for_err(illegal_argument_combination);
      return;
    }
    uint32_t destination_address = get_32b_uint();
    if (c_vm->v_cpu.f_regu[0])
      return;
    if (!verify_memory_address(destination_address + 3, data)) {
      load_reg_for_err(segmentation_violation);
      return;
    }
    c_vm->v_ram.container[destination_address] = (uint8_t)(data & 255);
    c_vm->v_ram.container[++destination_address] = (uint8_t)((data >> 8) & 255);
    c_vm->v_ram.container[++destination_address] =
        (uint8_t)((data >> 16) & 255);
    c_vm->v_ram.container[++destination_address] =
        (uint8_t)((data >> 24) & 255);
    break;
  }
  case 1: {
    uint8_t register_index = get_8b_uint();
    if (register_index > P_REGS_QUANTITY - 1) {
      load_reg_for_err(invalid_register);
      return;
    }
    c_vm->v_cpu.p_regs[register_index] = data;
    break;
  }
  }
}
static void (*isa_methods[14])(void) = {
    set_32b_inst, cipr_inst,  add_s_inst, add_u_inst, min_s_inst,
    min_u_inst,   mlt_s_inst, mlt_u_inst, div_s_inst, div_u_inst,
    and_inst,     or_inst,    xor_inst,   not_inst};
static uint8_t isa_size = sizeof(isa_methods) / sizeof(isa_methods[0]);

void sys_loop() {
  // load the first 16 bytes from v_disk
  // <1 byte> <1 byte> <4 bytes> <1 byte> <4 bytes>
  load_first_128b_from_disk();
  for (;;) {
    if (!c_vm->v_cpu.f_regu[2] && !c_vm->v_cpu.f_regu[0]) {
      uint8_t current_inst = get_8b_uint();
      if (current_inst <= isa_size - 1)
        isa_methods[current_inst]();
      else if (current_inst > isa_size) {
        c_vm->v_cpu.f_regu[0] = 1;
        c_vm->v_cpu.f_regu[1] = (uint8_t)invalid_instruction;
        c_vm->v_cpu.f_regu[2] = 1;
      } else if ((enum isa)current_inst == end) {
        terminate_current_process();
        change_current_process();
      }
    }
    if (c_vm->v_cpu.f_regu[0] && !c_vm->v_cpu.f_regu[2])
      c_vm->v_cpu.f_regu[2] = 1;
  }
}
