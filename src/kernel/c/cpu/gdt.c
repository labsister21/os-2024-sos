#include "cpu/gdt.h"
#include "cpu/interrupt.h"

const struct SegmentDescriptor null_segment = {
		.segment_low = 0,
		.segment_high = 0,

		.base_low = 0,
		.base_mid = 0,
		.base_high = 0,

		.type_bit = 0,
		.non_system = 0,
		.privilege_level = 0,
		.segment_present = 0,
		.contains_64 = 0,
		.param_flag = 0,
		.granularity = 0,

		.system_reserved = 0,
};

const struct SegmentDescriptor kernel_code_segment = {
		.segment_low = 0xFFFF,
		.segment_high = 0xF,

		.base_low = 0x0,
		.base_mid = 0x0,
		.base_high = 0x0,

		.type_bit = 0b1010,
		.non_system = 1,
		.privilege_level = 0,
		.segment_present = 1,
		.contains_64 = 0,
		.param_flag = 1,
		.granularity = 1,

		.system_reserved = 0,
};

const struct SegmentDescriptor kernel_data_segment = {
		.segment_low = 0xFFFF,
		.segment_high = 0xF,

		.base_low = 0,
		.base_mid = 0,
		.base_high = 0,

		.type_bit = 0b0010,
		.non_system = 1,
		.privilege_level = 0,
		.segment_present = 1,
		.contains_64 = 0,
		.param_flag = 1,
		.granularity = 1,

		.system_reserved = 0,
};

const struct SegmentDescriptor user_code_segment = {
		.segment_low = 0xFFFF,
		.segment_high = 0xF,

		.base_low = 0x0,
		.base_mid = 0x0,
		.base_high = 0x0,

		.type_bit = 0b1010,
		.non_system = 1,
		.privilege_level = 0x3,
		.segment_present = 1,
		.contains_64 = 0,
		.param_flag = 1,
		.granularity = 1,

		.system_reserved = 0,

};

const struct SegmentDescriptor user_data_segment = {
		.segment_low = 0xFFFF,
		.segment_high = 0xF,

		.base_low = 0,
		.base_mid = 0,
		.base_high = 0,

		.type_bit = 0b0010,
		.non_system = 1,
		.privilege_level = 0x3,
		.segment_present = 1,
		.contains_64 = 0,
		.param_flag = 1,
		.granularity = 1,

		.system_reserved = 0,
};

const struct SegmentDescriptor tss_segment = {
		.segment_high = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
		.segment_low = sizeof(struct TSSEntry),
		.base_low = 0,
		.base_mid = 0,
		.base_high = 0,
		.non_system = 0,
		.type_bit = 0x9,
		.privilege_level = 0,
		.segment_present = 1,
		.param_flag = 1,
		.contains_64 = 0,
		.granularity = 0
};

// clang-format off
/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual &
 * OSDev. Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data
 * (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
  .table = {
		null_segment, 
		kernel_code_segment, 
		kernel_data_segment,
		user_code_segment,
		user_data_segment, 
		tss_segment
	}
};
// clang-format on

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table
 * above. From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is
 * GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {.size = sizeof(global_descriptor_table), .address = &global_descriptor_table};

void gdt_install_tss(void) {
	uint32_t base = (uint32_t)&_interrupt_tss_entry;
	global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
	global_descriptor_table.table[5].base_mid = (base & (0xFF << 16)) >> 16;
	global_descriptor_table.table[5].base_low = base & 0xFFFF;
}
