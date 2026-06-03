#include <stdlib.h>
#include "world/map.h"

const TerrainStats TERRAIN_STATS[3] = {
    {2, 1, 0, 1}, // TERRAIN_PLAIN:    food+2 prod+1 def+0 sci+1
    {0, 2, 3, 0}, // TERRAIN_MOUNTAIN: food+0 prod+2 def+3 sci+0
    {1, 0, 0, 0}, // TERRAIN_WATER:    food+1 prod+0 def+0 sci+0
};

static TerrainType random_terrain(void)
{
    int r = rand() % 10;
    if (r < 6)
        return TERRAIN_PLAIN;
    if (r < 8)
        return TERRAIN_MOUNTAIN;
    return TERRAIN_WATER;
}

void map_init(GameState *gs)
{
    int w = gs->config.map_width;
    int h = gs->config.map_height;

    gs->map.width = w;
    gs->map.height = h;
    gs->map.grid = malloc(sizeof(Tile *) * h);
    for (int y = 0; y < h; y++) {
        gs->map.grid[y] = malloc(sizeof(Tile) * w);
        for (int x = 0; x < w; x++) {
            Tile *t = &gs->map.grid[y][x];
            t->x = x;
            t->y = y;
            t->type = TERRAIN_PLAIN;
            t->city_id = NO_ID;
            t->unit_id = NO_ID;
            t->religion_id = NO_ID;
            t->culture_owner = NO_ID;
        }
    }
}

void map_generate(GameState *gs)
{
    for (int y = 0; y < gs->map.height; y++) {
        for (int x = 0; x < gs->map.width; x++) {
            gs->map.grid[y][x].type = random_terrain();
        }
    }
}

Tile *map_get(GameState *gs, int x, int y)
{
    if (x < 0 || x >= gs->map.width)
        return NULL;
    if (y < 0 || y >= gs->map.height)
        return NULL;
    return &gs->map.grid[y][x];
}

void map_free(GameState *gs)
{
    for (int y = 0; y < gs->map.height; y++)
        free(gs->map.grid[y]);
    free(gs->map.grid);
    gs->map.grid = NULL;
    gs->map.width = 0;
    gs->map.height = 0;
}

int map_count_non_water(GameState *gs)
{
    int count = 0;

    for (int y = 0; y < gs->map.height; y++) {
        for (int x = 0; x < gs->map.width; x++) {
            if (gs->map.grid[y][x].type != TERRAIN_WATER)
                count++;
        }
    }
    return count;
}
