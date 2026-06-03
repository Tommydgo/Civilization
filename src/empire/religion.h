#ifndef RELIGION_H
#define RELIGION_H

#include "structs.h"

int religion_found(GameState *gs, int owner, const char *name);
void religion_spread_tick(GameState *gs);
int religion_count_tiles(GameState *gs, int religion_id);
bool religion_has_victory(GameState *gs);
int religion_of_owner(GameState *gs, int owner);

#endif
