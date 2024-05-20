#ifndef _MEMORY_H
#define _MEMORY_H

#include <std/string.h>
#define COPY_STRING_TO_LOCAL(name, from) \
	int from##_size = str_len(from) + 1;   \
	char name[from##_size];                \
	strcpy(name, from, from##_size)

#endif
