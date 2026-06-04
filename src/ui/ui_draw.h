#ifndef UI_DRAW_H
#define UI_DRAW_H

#include <raylib.h>
#include "structs.h"
#include "ui/ui_layout.h"

void draw_toolbar(Font font, GameState *gs);
void draw_map(Font font, GameState *gs, int selected_id, int mode);
void draw_panel(Font font, GameState *gs, int selected_id, int mode);
void draw_log(Font font, GameState *gs, int info_count,
              const char info_buf[][96]);

#endif
