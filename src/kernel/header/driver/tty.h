#ifndef _TTY_H
#define _TTY_H

// TODO: Create and use malloc
// TOOD: Option to ignore line edit layer
struct TTYState {
	int size;
	int current_read;
	char buffer[1024];

	int ansi_escape_size;
	char ansi_escape[32];
};

extern struct TTYState tty_state;

char fgetc();
void fputc(char c);

#endif
