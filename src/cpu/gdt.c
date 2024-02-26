#include "header/cpu/gdt.h"

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

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual &
 * OSDev. Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data
 * (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {null_segment, kernel_code_segment, kernel_data_segment}
};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table
 * above. From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is
 * GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table.
    // Use sizeof operator
};
