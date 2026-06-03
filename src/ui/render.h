#ifndef RENDER_H
#define RENDER_H

#include "structs.h"

void render_map(GameState *gs);
void render_status(GameState *gs);
void render_turn_start(GameState *gs);
void render_message(const char *fmt, ...);

#endif
