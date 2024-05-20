#ifndef _DEV_H
#define _DEV_H

#include "filesystem/vfs.h"
extern struct VFSHandler dev_vfs;

void fill_stdin_buffer(char c);

#endif
