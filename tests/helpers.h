#ifndef HELPERS_H
#define HELPERS_H

#include "structs.h"

// Cree un GameState minimal avec une carte NxN (toutes plaines) et l'empire joueur.
// N'utilise pas game_init() pour ne pas avoir de IA / ncurses / rand().
void gs_init_minimal(GameState *gs, int w, int h);
void gs_free_minimal(GameState *gs);

// Marque une technologie comme recherchee directement (sans passer par le tick).
void gs_give_tech(GameState *gs, int owner, int tech_id);

#endif
