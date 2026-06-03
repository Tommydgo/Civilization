#ifndef RENDER_H
#define RENDER_H

#include "structs.h"

// Lifecycle
void render_init(void);
void render_cleanup(void);

// Full TUI redraw (map + status + cities + units + events)
void render_full(GameState *gs);

// Reads one line of player input through the command bar
void render_read_input(char *buf, int len);

// Push a message into gs->events (shows in the events panel)
void render_message(GameState *gs, const char *fmt, ...);

// Info panel override — replaces the events panel content with custom lines.
// Call render_info_clear() to restore normal event display.
void render_info_clear(void);
void render_info_push(const char *fmt, ...);

// Ncurses menus — called from main.c before/after game loop.
// render_start_menu fills *config and *civ_id_out; returns 0.
int render_start_menu(GameConfig *config, int *civ_id_out);
// render_end_screen shows scores; returns true if player wants to replay.
bool render_end_screen(GameState *gs);

#endif
