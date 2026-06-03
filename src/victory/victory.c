#include "victory/victory.h"
#include "victory/score.h"
#include "empire/religion.h"

bool victory_check_science(GameState *gs)
{
    if (!gs->player.rocket.unlocked)
        return false;
    return gs->player.rocket.stages_completed >= ROCKET_TOTAL_STAGES;
}

bool victory_check_military(GameState *gs)
{
    // True if all active cities belong to a single owner
    int sole_owner = NO_ID;

    for (int i = 0; i < gs->cities.count; i++) {
        City *c = &gs->cities.data[i];
        if (!c->is_active)
            continue;
        if (sole_owner == NO_ID) {
            sole_owner = c->owner;
        } else if (c->owner != sole_owner) {
            return false;
        }
    }
    return sole_owner != NO_ID;
}

bool victory_check_religion(GameState *gs)
{
    return religion_has_victory(gs);
}

bool victory_check_score(GameState *gs)
{
    return gs->current_turn >= gs->config.max_turns;
}

void victory_check(GameState *gs)
{
    if (gs->victory.achieved)
        return;
    bool achieved = false;
    VictoryType type = gs->config.victory_type;
    int winner = NO_ID;

    switch (type) {
    case VICTORY_SCIENCE:
        if (victory_check_science(gs)) {
            achieved = true;
            winner = PLAYER_OWNER_ID;
        }
        break;
    case VICTORY_MILITARY:
        if (victory_check_military(gs)) {
            // Find the single owner who holds all cities
            for (int i = 0; i < gs->cities.count; i++) {
                if (gs->cities.data[i].is_active) {
                    winner = gs->cities.data[i].owner;
                    break;
                }
            }
            if (winner != NO_ID)
                achieved = true;
        }
        break;
    case VICTORY_RELIGION:
        if (victory_check_religion(gs)) {
            // Find the religion that triggered victory
            int non_water = 0;
            for (int y = 0; y < gs->map.height; y++) {
                for (int x = 0; x < gs->map.width; x++) {
                    if (gs->map.grid[y][x].type != TERRAIN_WATER)
                        non_water++;
                }
            }
            for (int i = 0; i < gs->religions.count; i++) {
                int tiles = gs->religions.data[i].converted_tiles;
                if (non_water > 0 && tiles * 100 / non_water >= RELIGION_VICTORY_THRESHOLD) {
                    winner = gs->religions.data[i].founder_owner;
                    achieved = true;
                    break;
                }
            }
        }
        break;
    case VICTORY_SCORE:
        if (victory_check_score(gs)) {
            score_update_all(gs);
            winner = score_find_winner(gs);
            achieved = true;
        }
        break;
    }
    if (achieved) {
        gs->victory.achieved = true;
        gs->victory.type = type;
        gs->victory.winner_owner = winner;
        gs->game_over = true;
    }
}
