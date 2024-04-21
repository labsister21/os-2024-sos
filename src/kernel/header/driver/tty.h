#ifndef _TTY_H
#define _TTY_H

// TODO: Create and use malloc
// TOOD: Option to ignore line edit layer
struct TTYState {
	int size;
	int current;
	char buffer[1024];
};

extern struct TTYState tty_state;

char fgetc();

#endif
