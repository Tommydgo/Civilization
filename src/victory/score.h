#ifndef SCORE_H
#define SCORE_H

#include "structs.h"

int score_calc_player(GameState *gs);
int score_calc_faction(GameState *gs, int faction_idx);
void score_update_all(GameState *gs);
int score_find_winner(GameState *gs);

#endif
