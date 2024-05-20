#ifndef __PATH_H
#define __PATH_H

int resolve_path(char *path);
int split_path(char *path, char **dirname, char **basename);

#endif
