#include <stdlib.h>
#include <string.h>
#include "helpers.h"
#include "structs.h"
#include "constants.h"
#include "empire/empire.h"
#include "tech/tech.h"

void gs_init_minimal(GameState *gs, int w, int h)
{
    memset(gs, 0, sizeof(GameState));
    gs->current_turn = 1;
    gs->config.map_width = w;
    gs->config.map_height = h;
    gs->config.num_ai_factions = 0;
    gs->config.difficulty = DIFF_MEDIUM;
    gs->config.max_turns = 200;
    gs->config.victory_type = VICTORY_SCORE;
    gs->victory.achieved = false;
    gs->game_over = false;

    UnitArray_init(&gs->units);
    CityArray_init(&gs->cities);
    AIFactionArray_init(&gs->ai_factions);
    ReligionArray_init(&gs->religions);
    GameEventArray_init(&gs->events);

    gs->map.width = w;
    gs->map.height = h;
    gs->map.grid = malloc((size_t)h * sizeof(Tile *));
    for (int y = 0; y < h; y++) {
        gs->map.grid[y] = malloc((size_t)w * sizeof(Tile));
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

    empire_init(gs);
}

void gs_free_minimal(GameState *gs)
{
    for (int i = 0; i < gs->cities.count; i++)
        CityBuildingArray_free(&gs->cities.data[i].buildings);
    CityArray_free(&gs->cities);
    UnitArray_free(&gs->units);
    for (int i = 0; i < gs->ai_factions.count; i++)
        TechStateArray_free(&gs->ai_factions.data[i].techs);
    AIFactionArray_free(&gs->ai_factions);
    ReligionArray_free(&gs->religions);
    GameEventArray_free(&gs->events);
    empire_free(gs);
    for (int y = 0; y < gs->map.height; y++)
        free(gs->map.grid[y]);
    free(gs->map.grid);
}

void gs_give_tech(GameState *gs, int owner, int tech_id)
{
    TechStateArray *arr = (owner == PLAYER_OWNER_ID)
        ? &gs->player.techs : NULL;
    if (owner != PLAYER_OWNER_ID) {
        for (int i = 0; i < gs->ai_factions.count; i++) {
            if (gs->ai_factions.data[i].id == owner) {
                arr = &gs->ai_factions.data[i].techs;
                break;
            }
        }
    }
    if (!arr)
        return;
    for (int i = 0; i < arr->count; i++) {
        if (arr->data[i].tech_id == tech_id) {
            arr->data[i].researched = true;
            break;
        }
    }
}
