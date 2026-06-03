#ifndef TECH_H
#define TECH_H

#include "structs.h"

void tech_init_owner(GameState *gs, int owner);
bool tech_can_research(GameState *gs, int owner, int tech_id);
void tech_research_tick(GameState *gs, int owner);
void tech_apply_unlock(GameState *gs, int owner, TechUnlock unlock);
bool tech_is_researched(GameState *gs, int owner, int tech_id);

#endif
