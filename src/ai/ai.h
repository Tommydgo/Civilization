#ifndef AI_H
#define AI_H

#include "structs.h"

void ai_faction_init(GameState *gs, int faction_idx);
void ai_tick(GameState *gs, int faction_idx);
void ai_spawn_unit(GameState *gs, int faction_idx);
void ai_try_attack(GameState *gs, int faction_idx);

#endif
