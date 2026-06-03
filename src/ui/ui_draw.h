#ifndef UI_DRAW_H
#define UI_DRAW_H
#include <SFML/Graphics.h>
#include "structs.h"
#include "ui/ui_layout.h"

void draw_toolbar(sfRenderWindow *win, sfFont *font, GameState *gs);
void draw_map(sfRenderWindow *win, sfFont *font, GameState *gs,
              int selected_id, int mode);
void draw_panel(sfRenderWindow *win, sfFont *font, GameState *gs,
                int selected_id, int mode);
void draw_log(sfRenderWindow *win, sfFont *font, GameState *gs,
              int info_count, const char info_buf[][96]);
#endif
