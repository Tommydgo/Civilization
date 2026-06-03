#ifndef SAVE_H
#define SAVE_H

#include "structs.h"

bool save_write(GameState *gs, const char *path);
bool save_read(GameState *gs, const char *path);
void save_list(const char *dir);

#endif
