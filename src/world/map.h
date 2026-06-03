#ifndef MAP_H
#define MAP_H

#include "structs.h"

extern const TerrainStats TERRAIN_STATS[3];

void map_init(GameState *gs);
void map_generate(GameState *gs);
Tile *map_get(GameState *gs, int x, int y);
void map_free(GameState *gs);
int map_count_non_water(GameState *gs);

#endif
