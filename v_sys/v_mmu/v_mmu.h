#ifndef V_MMU_H
#define V_MMU_H
#include "../globals.h"
#include <stdint.h>
void terminate_current_process();
void cprc_inst(uint32_t init_pos, uint32_t file_length);
void change_current_process();
enum v_ram_segments : uint8_t { text, data, heap, stack };
uint8_t verify_memory_address(uint32_t address,
                              enum v_ram_segments segment_type);
#endif
