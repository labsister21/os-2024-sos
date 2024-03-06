ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS {
	. = 0x00000000; 

	.text ALIGN(4):
	{
		bin/obj/program/crt0.o(.text)
		*(.text)
	}

	.data ALIGN(4):
	{
		*(.data)
	}

	.rodata ALIGN(4):
	{
		*(.rodata*)
	}

	_linker_user_program_end = .;
	ASSERT ((_linker_user_program_end <= 1 * 1024 * 1024), "Error: User program linking result is more than 1 MiB")
}
