#ifndef V_MMU_H
#define V_MMU_H
#include <stdint.h> 
#include "../globals.h"
void change_current_process(); 
enum v_ram_segments : uint8_t {
  text, data, heap, stack
}; 
uint8_t verify_memory_address(uint32_t address, enum v_ram_segments segment_type);
#endif