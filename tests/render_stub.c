
#include <stdarg.h>
#include "ui/render.h"

void render_init(void) {}
void render_cleanup(void) {}
void render_full(GameState *gs) { (void)gs; }
void render_read_input(char *buf, int len) { (void)buf; (void)len; }
void render_info_clear(void) {}
void render_info_push(const char *fmt, ...) { (void)fmt; }
void render_message(GameState *gs, const char *fmt, ...) { (void)gs; (void)fmt; }
int  render_start_menu(GameConfig *c, int *civ) { (void)c; (void)civ; return 0; }
bool render_end_screen(GameState *gs) { (void)gs; return false; }
