#ifndef UNIT_H
#define UNIT_H

#include "structs.h"

extern const UnitTemplate UNIT_TEMPLATES[];
extern const int UNIT_TEMPLATE_COUNT;

int unit_create(GameState *gs, int template_id, int x, int y, int owner);
bool unit_move(GameState *gs, int unit_id, int tx, int ty);
void unit_attack(GameState *gs, int attacker_id, int defender_id);
void unit_kill(GameState *gs, int unit_id);
const UnitTemplate *unit_template_get(int id);
Unit *unit_get(GameState *gs, int unit_id);
void unit_reset_id_counter(GameState *gs);

#endif
