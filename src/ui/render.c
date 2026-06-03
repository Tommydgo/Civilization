#include <SFML/Graphics.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ui/render.h"
#include "ui/ui_layout.h"
#include "ui/ui_draw.h"
#include "ui/ui_dialog.h"
#include "structs.h"
#include "constants.h"

/* ── UI state machine ── */
typedef enum {
    UI_IDLE,
    UI_UNIT_SELECTED,
    UI_MOVE_MODE,
    UI_ATTACK_MODE,
    UI_DIALOG_INPUT,
    UI_COMMAND_READY
} UIMode;

/* ── Static module state ── */
static sfRenderWindow *s_win   = NULL;
static sfFont         *s_font  = NULL;
static int             s_active = 0;

static UIMode s_mode        = UI_IDLE;
static int    s_selected_id = NO_ID;
static char   s_cmd_buf[128] = {0};

#define INFO_LINES_MAX 40
static char s_info_buf[INFO_LINES_MAX][96];
static int  s_info_count = 0;

static GameState *s_gs = NULL;

/* draw_text helper used by render_end_screen */
static void draw_text(sfRenderWindow *w, sfFont *f, const char *str,
                      float x, float y, unsigned int sz, sfColor col)
{
    sfText *t = sfText_create();
    sfText_setFont(t, f);
    sfText_setString(t, str);
    sfText_setCharacterSize(t, sz);
    sfText_setFillColor(t, col);
    sfText_setPosition(t, (sfVector2f){x, y});
    sfRenderWindow_drawText(w, t, NULL);
    sfText_destroy(t);
}

/* ── Public API ── */

void render_init(void)
{
    sfVideoMode mode = {WIN_W, WIN_H, 32};
    s_win = sfRenderWindow_create(mode, "Civilization", sfDefaultStyle, NULL);
    if (!s_win)
        return;
    s_font = sfFont_createFromFile("assets/font.ttf");
    if (!s_font) {
        fprintf(stderr, "render: cannot load assets/font.ttf\n");
        sfRenderWindow_destroy(s_win);
        s_win = NULL;
        return;
    }
    s_active = 1;
}

void render_cleanup(void)
{
    if (!s_active)
        return;
    if (s_font) sfFont_destroy(s_font);
    if (s_win)  sfRenderWindow_destroy(s_win);
    s_win    = NULL;
    s_font   = NULL;
    s_active = 0;
}

void render_info_clear(void)
{
    s_info_count = 0;
}

void render_info_push(const char *fmt, ...)
{
    if (s_info_count >= INFO_LINES_MAX)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_info_buf[s_info_count++], 95, fmt, ap);
    va_end(ap);
}

void render_message(GameState *gs, const char *fmt, ...)
{
    if (!gs)
        return;
    GameEvent ev;
    ev.type  = EVENT_UNIT_KILLED;
    ev.owner = PLAYER_OWNER_ID;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(ev.msg, sizeof(ev.msg), fmt, ap);
    va_end(ap);
    GameEventArray_push(&gs->events, ev);
}

void render_full(GameState *gs)
{
    s_gs = gs;
    if (!s_active || !s_win)
        return;
    sfRenderWindow_clear(s_win, COL_BG);
    draw_toolbar(s_win, s_font, gs);
    draw_map(s_win, s_font, gs, s_selected_id, s_mode);
    draw_panel(s_win, s_font, gs, s_selected_id, s_mode);
    draw_log(s_win, s_font, gs, s_info_count,
             (const char (*)[96])s_info_buf);
    sfRenderWindow_display(s_win);
}

/* render_read_input, render_start_menu, render_end_screen
   implemented in Tasks 10, 11 */
void render_read_input(char *buf, int len)
{
    (void)buf; (void)len;
}

int render_start_menu(GameConfig *config, int *civ_id_out)
{
    (void)config; (void)civ_id_out;
    return 0;
}

bool render_end_screen(GameState *gs)
{
    (void)gs;
    return false;
}
