#ifndef VICTORY_H
#define VICTORY_H

#include "structs.h"

void victory_check(GameState *gs);
bool victory_check_science(GameState *gs);
bool victory_check_military(GameState *gs);
bool victory_check_religion(GameState *gs);
bool victory_check_score(GameState *gs);

#endif
