
#include "sbush_process.h"

#ifndef EXEC_H
#define EXEC_H

extern char **environ;

void execute(Job job);
int _strcmp(char* str, char* str2);
int _extract_env(char* env_str, char* key, char* value);
void _write_env(char* key, char* value, char* output);
int _search_env(char** env, char* key_to_find);

#endif
