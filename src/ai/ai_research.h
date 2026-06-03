#ifndef AI_RESEARCH_H
#define AI_RESEARCH_H

#include "structs.h"

void ai_research_tick(GameState *gs, AIFaction *faction);
int ai_select_next_tech(GameState *gs, AIFaction *faction);
bool ai_tech_is_military(int tech_id);

#endif
