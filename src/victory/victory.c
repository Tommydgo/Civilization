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
    
    if (gs->ai_factions.count == 0)
        return false;
    
    for (int i = 0; i < gs->ai_factions.count; i++) {
        if (!gs->ai_factions.data[i].is_eliminated)
            return false;
    }
    
    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].is_active
                && gs->cities.data[i].owner == PLAYER_OWNER_ID)
            return true;
    }
    return false;
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

    if (victory_check_science(gs)) {
        gs->victory.achieved = true;
        gs->victory.type = VICTORY_SCIENCE;
        gs->victory.winner_owner = PLAYER_OWNER_ID;
        gs->game_over = true;
        return;
    }

    if (victory_check_military(gs)) {
        for (int i = 0; i < gs->cities.count; i++) {
            if (gs->cities.data[i].is_active) {
                gs->victory.achieved = true;
                gs->victory.type = VICTORY_MILITARY;
                gs->victory.winner_owner = gs->cities.data[i].owner;
                gs->game_over = true;
                return;
            }
        }
    }

    if (victory_check_religion(gs)) {
        int non_water = 0;
        for (int y = 0; y < gs->map.height; y++)
            for (int x = 0; x < gs->map.width; x++)
                if (gs->map.grid[y][x].type != TERRAIN_WATER)
                    non_water++;
        for (int i = 0; i < gs->religions.count; i++) {
            int tiles = gs->religions.data[i].converted_tiles;
            if (non_water > 0 && tiles * 100 / non_water >= RELIGION_VICTORY_THRESHOLD) {
                gs->victory.achieved = true;
                gs->victory.type = VICTORY_RELIGION;
                gs->victory.winner_owner = gs->religions.data[i].founder_owner;
                gs->game_over = true;
                return;
            }
        }
    }

    if (victory_check_score(gs)) {
        gs->victory.achieved = true;
        gs->victory.type = VICTORY_SCORE;
        gs->victory.winner_owner = score_find_winner(gs);
        gs->game_over = true;
    }
}
