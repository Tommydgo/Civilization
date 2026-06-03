#include "victory/victory.h"
#include "victory/score.h"
#include "empire/religion.h"
#include "world/map.h"

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

static int winner_science(GameState *gs)
{
    (void)gs;
    return PLAYER_OWNER_ID;
}

static int winner_military(GameState *gs)
{
    int i = 0;

    while (i < gs->cities.count) {
        if (gs->cities.data[i].is_active)
            return gs->cities.data[i].owner;
        i++;
    }
    return PLAYER_OWNER_ID;
}

static int winner_religion(GameState *gs)
{
    int non_water = map_count_non_water(gs);
    int i = 0;

    while (i < gs->religions.count) {
        int tiles = gs->religions.data[i].converted_tiles;
        if (non_water > 0 && tiles * 100 / non_water >= RELIGION_VICTORY_THRESHOLD)
            return gs->religions.data[i].founder_owner;
        i++;
    }
    return PLAYER_OWNER_ID;
}

typedef struct {
    VictoryType type;
    bool        (*check)(GameState *);
    int         (*winner)(GameState *);
} VictoryEntry;

static const VictoryEntry VICTORIES[] = {
    { VICTORY_SCIENCE,  victory_check_science,  winner_science    },
    { VICTORY_MILITARY, victory_check_military, winner_military   },
    { VICTORY_RELIGION, victory_check_religion, winner_religion   },
    { VICTORY_SCORE,    victory_check_score,    score_find_winner },
};

static const int VICTORY_COUNT = 4;

void victory_check(GameState *gs)
{
    int i = 0;

    if (gs->victory.achieved)
        return;
    while (i < VICTORY_COUNT) {
        if (VICTORIES[i].check(gs)) {
            gs->victory.achieved = true;
            gs->victory.type = VICTORIES[i].type;
            gs->victory.winner_owner = VICTORIES[i].winner(gs);
            gs->game_over = true;
            return;
        }
        i++;
    }
}
