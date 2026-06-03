#ifndef TILE_H
#define TILE_H

#include "structs.h"

void tile_set_culture(GameState *gs, int x, int y, int owner);
void tile_set_religion(GameState *gs, int x, int y, int religion_id);
void tile_set_unit(GameState *gs, int x, int y, int unit_id);
bool tile_is_passable(GameState *gs, int x, int y, int unit_id);

#endif
