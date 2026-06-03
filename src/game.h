#ifndef GAME_H
#define GAME_H

#include "structs.h"

void game_init(GameState *gs, GameConfig config);
void game_run(GameState *gs);
void game_tick(GameState *gs);
void game_dispatch(GameState *gs, Command cmd);
void game_free(GameState *gs);

#endif
