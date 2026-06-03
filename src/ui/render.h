#ifndef RENDER_H
#define RENDER_H

#include "structs.h"

void render_init(void);
void render_cleanup(void);

void render_full(GameState *gs);

void render_read_input(char *buf, int len);

void render_message(GameState *gs, const char *fmt, ...);

void render_info_clear(void);
void render_info_push(const char *fmt, ...);

int render_start_menu(GameConfig *config, int *civ_id_out);

bool render_end_screen(GameState *gs);

#endif
