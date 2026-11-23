#include <stdint.h>
enum shared_idt : uint8_t {
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
  segmentation_violation,
  invalid_operator_argument,
  illegal_argument_combination,
  attempted_access_to_uninit_disk,
  file_access_failed,
  pid_queue_full,
  ran_out_of_memory
};

void load_reg_for_err(enum shared_idt x);
void sys_loop();
