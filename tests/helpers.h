#ifndef HELPERS_H
#define HELPERS_H

#include "structs.h"

void gs_init_minimal(GameState *gs, int w, int h);
void gs_free_minimal(GameState *gs);

void gs_give_tech(GameState *gs, int owner, int tech_id);

#endif
