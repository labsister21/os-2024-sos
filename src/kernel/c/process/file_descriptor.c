#include "process/file_descriptor.h"
#include "filesystem/vfs.h"
#include "process/scheduler.h"
#include "text/framebuffer.h"

#define GET_CURRENT_PID(name)           \
	int                                   \
			name = get_current_running_pid(); \
	if (name < 0)                         \
	return -1

#define CHECK_ERROR(status) \
	if (status < 0)           \
	return status

int get_free_fd_of_current_process() {
	GET_CURRENT_PID(pid);
	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);

	int fd = 0;
	while (fd < PROCESS_MAX_FD) {
		if (pcb->fd[fd] == -1) break;
		fd += 1;
	}

	return fd;
};

int set_ft_of_current_process(int fd, int ft) {
	CHECK_ERROR(fd);
	GET_CURRENT_PID(pid);
	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
	pcb->fd[fd] = ft;
	return 0;
};

int get_ft_of_current_process(int fd) {
	CHECK_ERROR(fd);
	GET_CURRENT_PID(pid);
	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
	return pcb->fd[fd];
}

int clear_fd_of_current_process(int fd) {
	CHECK_ERROR(fd);
	GET_CURRENT_PID(pid);
	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
	pcb->fd[fd] = -1;
	return 0;
};

void cleanup_fd(struct ProcessControlBlock *pcb) {
	for (int i = 0; i < PROCESS_MAX_FD; ++i) {
		int ft = pcb->fd[i];
		if (ft == -1) continue;
		get_vfs_table_entry(ft);
		vfs.close(ft);
	}
}
