#include "header/cpu/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // TODO : Implement
            .segment_low = 0,
            .base_low  = 0,
            .base_mid = 0,
            .type_bit = 0,
            .non_system = 0,
            .dpl = 0,
            .p = 0,
            .seqlimit = 0,
            .avl = 0,
            .l = 0,
            .db = 0,
            .g = 0,
            .base_end = 0,
        },
        {
            // TODO : Implement
            .segment_low = 0xFFFF,
            .base_low  = 0,
            .base_mid = 0,
            .type_bit = 0xA,
            .non_system = 1,
            .dpl = 0,
            .p = 1,
            .seqlimit = 0xF,
            .avl = 1,
            .l = 0,
            .db = 1,
            .g = 1,
            .base_end = 0,

        },
        {
            // TODO : Implement
            .segment_low = 0xFFFF,
            .base_low  = 0,
            .base_mid = 0,
            .type_bit = 0x2,
            .non_system = 1,
            .dpl = 0,
            .p = 1,
            .seqlimit = 0xF,
            .avl = 1,
            .l = 0,
            .db = 1,
            .g = 1,
            .base_end = 0,
        }
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table. 
    //        Use sizeof operator
    .size = sizeof(global_descriptor_table.table) - 1,
    .address = &global_descriptor_table
};
