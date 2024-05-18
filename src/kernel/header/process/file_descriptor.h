#ifndef _FILE_DESCRIPTOR_H
#define _FILE_DESCRIPTOR_H

#include "process/process.h"
int get_free_fd_of_current_process();
int get_ft_of_current_process(int fd);
int set_ft_of_current_process(int fd, int ft);
int clear_fd_of_current_process(int fd);

void cleanup_fd(struct ProcessControlBlock *pcb);

#endif
