#include <path.h>
#include <std/string.h>

int resolve_path(char *path) {
	if (path[0] != '/') return -1;

	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	int separator_count = 0;
	for (int i = 0; i < size; ++i) {
		if (copy[i] == '/') separator_count += 1;
	}

	char *separated[separator_count];
	int count = 0;

	char *token = strtok(copy, '/');
	while (token != NULL) {
		separated[count++] = token;
		token = strtok(NULL, '/');
	}

	int current_stack = 0;
	int stack[separator_count];

	path[0] = '\0';
	int current_idx = 0;
	for (int i = 0; i < count; ++i) {
		char *current = separated[i];
		if (strcmp(current, ".") == 0) {
			continue;
		} else if (strcmp(current, "..") == 0) {
			if (current_stack == 0) {
				current_idx = 0;
			} else {
				current_stack -= 1;
				current_idx = stack[current_stack];
			}
			path[current_idx] = '\0';

		} else {
			int j = 0;
			stack[current_stack++] = current_idx;
			path[current_idx++] = '/';
			while (current[j] != '\0') {
				path[current_idx++] = current[j++];
			}
			path[current_idx] = '\0';
		}
	}

	if (path[0] == '\0')
		strcpy(path, "/", 2);

	return 0;
}

int split_path(char *path, char **dirname, char **basename) {
	int size = str_len(path);
	int idx = size - 1;
	while (idx >= 0) {
		if (path[idx] == '/') break;
		idx -= 1;
	}
	if (idx == -1) return -1;

	path[idx] = '\0';
	*dirname = path;
	*basename = &path[idx + 1];
	if (idx == 0)
		*dirname = "/";
	return 0;
}
