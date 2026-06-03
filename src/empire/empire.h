#ifndef EMPIRE_H
#define EMPIRE_H

#include "structs.h"

void empire_init(GameState *gs);
void empire_tick(GameState *gs);
void empire_apply_ability(GameState *gs, SpecialAbility ability);
void empire_free(GameState *gs);

// Owner-agnostic accessors — work for the player (PLAYER_OWNER_ID) and AI factions
bool *owner_abilities(GameState *gs, int owner);
TechStateArray *owner_techs(GameState *gs, int owner);
ResearchQueue *owner_research(GameState *gs, int owner);
RocketProject *owner_rocket(GameState *gs, int owner);
int *owner_score_ptr(GameState *gs, int owner);

#endif
