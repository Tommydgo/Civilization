#include "victory/score.h"
#include "empire/empire.h"

static int count_owner_units(GameState *gs, int owner)
{
    int count = 0;

    for (int i = 0; i < gs->units.count; i++) {
        if (gs->units.data[i].is_active && gs->units.data[i].owner == owner)
            count++;
    }
    return count;
}

static int count_owner_pop(GameState *gs, int owner)
{
    int total = 0;

    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].is_active && gs->cities.data[i].owner == owner)
            total += gs->cities.data[i].population;
    }
    return total;
}

static int count_culture_tiles(GameState *gs, int owner)
{
    int count = 0;

    for (int y = 0; y < gs->map.height; y++) {
        for (int x = 0; x < gs->map.width; x++) {
            if (gs->map.grid[y][x].culture_owner == owner)
                count++;
        }
    }
    return count;
}

static int compute_score(int pop, int units, int tiles, int culture)
{
    return pop + units * 2 + tiles * 3 + culture / 10;
}

int score_calc_player(GameState *gs)
{
    int pop = count_owner_pop(gs, PLAYER_OWNER_ID);
    int units = count_owner_units(gs, PLAYER_OWNER_ID);
    int tiles = count_culture_tiles(gs, PLAYER_OWNER_ID);
    return compute_score(pop, units, tiles, gs->player.culture_points);
}

int score_calc_faction(GameState *gs, int faction_idx)
{
    AIFaction *f = &gs->ai_factions.data[faction_idx];
    int pop = count_owner_pop(gs, f->id);
    int units = count_owner_units(gs, f->id);
    int tiles = count_culture_tiles(gs, f->id);
    return compute_score(pop, units, tiles, f->score);
}

void score_update_all(GameState *gs)
{
    gs->player.score = score_calc_player(gs);
    for (int i = 0; i < gs->ai_factions.count; i++) {
        if (!gs->ai_factions.data[i].is_eliminated)
            gs->ai_factions.data[i].score = score_calc_faction(gs, i);
    }
}

int score_find_winner(GameState *gs)
{
    int best_score = gs->player.score;
    int best_owner = PLAYER_OWNER_ID;

    for (int i = 0; i < gs->ai_factions.count; i++) {
        AIFaction *f = &gs->ai_factions.data[i];
        if (!f->is_eliminated && f->score > best_score) {
            best_score = f->score;
            best_owner = f->id;
        }
    }
    return best_owner;
}
