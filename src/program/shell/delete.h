#include <fat32.h>
#include <path.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>
#include <vfs.h>

#include "util.h"
#define MAX_PATH 1024

#ifndef __DELETE_H
#define __DELETE_H

void delete_recursive(char *init_path);

#endif