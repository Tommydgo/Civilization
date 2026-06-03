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
#include "entities/unit.h"
#include "entities/city.h"
#include "tech/tech_tree.h"

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

/* ── Static helpers ── */

static bool screen_to_tile(GameState *gs, int px, int py, int *tx, int *ty)
{
    if (px < MAP_X || px >= MAP_X + MAP_W) return false;
    if (py < MAP_Y || py >= MAP_Y + MAP_H) return false;
    float tw = (float)MAP_W / gs->map.width;
    float th = (float)MAP_H / gs->map.height;
    *tx = (int)((px - MAP_X) / tw);
    *ty = (int)((py - MAP_Y) / th);
    return (*tx >= 0 && *tx < gs->map.width &&
            *ty >= 0 && *ty < gs->map.height);
}

static int unit_at(GameState *gs, int tx, int ty)
{
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (u->is_active && u->owner == PLAYER_OWNER_ID
                         && u->x == tx && u->y == ty)
            return u->id;
    }
    return NO_ID;
}

static int enemy_at(GameState *gs, int tx, int ty)
{
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (u->is_active && u->owner != PLAYER_OWNER_ID
                         && u->x == tx && u->y == ty)
            return u->id;
    }
    return NO_ID;
}

void render_read_input(char *buf, int len)
{
    if (!s_active || !s_win || !s_gs) {
        buf[0] = '\0';
        return;
    }

    sfEvent ev;
    while (sfRenderWindow_isOpen(s_win)) {
        render_full(s_gs);

        while (sfRenderWindow_pollEvent(s_win, &ev)) {
            if (ev.type == sfEvtClosed) {
                strncpy(s_cmd_buf, "quit", sizeof(s_cmd_buf));
                s_mode = UI_COMMAND_READY;
            }

            if (ev.type == sfEvtMouseButtonPressed &&
                ev.mouseButton.button == sfMouseLeft) {
                int mx = ev.mouseButton.x;
                int my = ev.mouseButton.y;

                if (IN_RECT(mx, my, BTN_NEXT_X, BTN_Y, BTN_NEXT_W, BTN_H)) {
                    strncpy(s_cmd_buf, "next", sizeof(s_cmd_buf));
                    s_mode = UI_COMMAND_READY;
                } else if (IN_RECT(mx, my, BTN_RECH_X, BTN_Y,
                                   BTN_RECH_W, BTN_H)) {
                    char tech[64] = {0};
                    dialog_text_input(s_win, s_font,
                                      "Nom de la technologie :", tech, 64);
                    if (tech[0]) {
                        snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                                 "research %s", tech);
                        s_mode = UI_COMMAND_READY;
                    }
                } else if (IN_RECT(mx, my, BTN_SAVE_X, BTN_Y,
                                   BTN_SAVE_W, BTN_H)) {
                    char fname[64] = {0};
                    dialog_text_input(s_win, s_font,
                                      "Nom du fichier de sauvegarde :",
                                      fname, 64);
                    if (fname[0]) {
                        snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                                 "save %s", fname);
                        s_mode = UI_COMMAND_READY;
                    }
                } else if (IN_RECT(mx, my, BTN_LOAD_X, BTN_Y,
                                   BTN_LOAD_W, BTN_H)) {
                    char fname[64] = {0};
                    dialog_text_input(s_win, s_font,
                                      "Nom du fichier a charger :",
                                      fname, 64);
                    if (fname[0]) {
                        snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                                 "load %s", fname);
                        s_mode = UI_COMMAND_READY;
                    }
                } else if (mx >= PANEL_X && s_mode == UI_UNIT_SELECTED) {
                    if (IN_RECT(mx, my, PBTN_X, PBTN_MOVE_Y,
                                PBTN_W, PBTN_H)) {
                        s_mode = UI_MOVE_MODE;
                    } else if (IN_RECT(mx, my, PBTN_X, PBTN_ATK_Y,
                                       PBTN_W, PBTN_H)) {
                        s_mode = UI_ATTACK_MODE;
                    } else if (IN_RECT(mx, my, PBTN_X, PBTN_FOUND_Y,
                                       PBTN_W, PBTN_H)) {
                        char cname[64] = {0};
                        dialog_text_input(s_win, s_font,
                                          "Nom de la ville :", cname, 64);
                        if (cname[0]) {
                            snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                                     "create_city %s", cname);
                            s_mode = UI_COMMAND_READY;
                        }
                    }
                } else {
                    int tx, ty;
                    if (screen_to_tile(s_gs, mx, my, &tx, &ty)) {
                        if (s_mode == UI_MOVE_MODE) {
                            snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                                     "move %d %d %d",
                                     s_selected_id, tx, ty);
                            s_mode = UI_COMMAND_READY;
                            s_selected_id = NO_ID;
                        } else if (s_mode == UI_ATTACK_MODE) {
                            int eid = enemy_at(s_gs, tx, ty);
                            if (eid != NO_ID) {
                                snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                                         "attack %d %d",
                                         s_selected_id, eid);
                                s_mode = UI_COMMAND_READY;
                                s_selected_id = NO_ID;
                            }
                        } else {
                            int uid = unit_at(s_gs, tx, ty);
                            if (uid != NO_ID) {
                                s_selected_id = uid;
                                s_mode = UI_UNIT_SELECTED;
                            } else {
                                s_selected_id = NO_ID;
                                s_mode = UI_IDLE;
                            }
                        }
                    }
                }
            }

            if (ev.type == sfEvtKeyPressed &&
                ev.key.code == sfKeyEscape) {
                s_selected_id = NO_ID;
                s_mode = UI_IDLE;
            }
        }

        if (s_mode == UI_COMMAND_READY) {
            strncpy(buf, s_cmd_buf, (size_t)(len - 1));
            buf[len - 1] = '\0';
            s_mode = UI_IDLE;
            return;
        }
    }
    strncpy(buf, "quit", (size_t)(len - 1));
}

int render_start_menu(GameConfig *config, int *civ_id_out)
{
    if (!s_active || !s_win) {
        *civ_id_out = 0;
        return 0;
    }

    const char *diff[] = {
        "Peaceful  - 0 IA  (exploration libre)",
        "Easy      - 1 IA  (aggression faible)",
        "Medium    - 3 IA  (aggression moderee)",
        "Hard      - 5 IA  (+ tech de depart)",
        "Extreme   - 7 IA  (+ tech + aggression max)"
    };
    int diff_ai[] = {0, 1, 3, 5, 7};
    int sel = dialog_list_select(s_win, s_font,
                                 "Choisissez la difficulte",
                                 diff, 5, 2);
    if (sel < 0) sel = 2;
    config->difficulty      = sel;
    config->num_ai_factions = diff_ai[sel];

    const char *civs[] = {
        "Rome        - +1 Guerrier, +1 attaque permanente",
        "Egypte      - +1 production dans toutes les villes",
        "Grece       - Ecriture offerte, +2 science/tour",
        "Mongolie    - Guerre offerte, +1 mouvement aux unites",
        "Chine       - Frontieres culturelles, +5 culture/tour",
        "Azteques    - Religion disponible + 1 Missionnaire",
        "Angleterre  - Settlers traversent l'eau",
        "Allemagne   - Industrie offerte + 10 or de depart",
        "Japon       - +2 attaque et +1 defense permanentes",
        "Inde        - +2 nourriture dans toutes les villes"
    };
    sel = dialog_list_select(s_win, s_font,
                             "Choisissez votre civilisation",
                             civs, 10, 0);
    if (sel < 0) sel = 0;
    *civ_id_out    = sel;
    config->civ_id = sel;
    return 0;
}

bool render_end_screen(GameState *gs)
{
    if (!s_active || !s_win)
        return false;

    sfEvent ev;
    while (sfRenderWindow_isOpen(s_win)) {
        while (sfRenderWindow_pollEvent(s_win, &ev)) {
            if (ev.type == sfEvtClosed)
                return false;
            if (ev.type == sfEvtKeyPressed) {
                if (ev.key.code == sfKeyEscape) return false;
                if (ev.key.code == sfKeyR)      return true;
                if (ev.key.code == sfKeyQ)      return false;
            }
        }

        sfRenderWindow_clear(s_win, sfColor_fromRGB(10, 10, 20));

        char buf[256];
        float col = WIN_W / 5.f;
        float row = 40;

        snprintf(buf, sizeof(buf), "=== PARTIE TERMINEE - Tour %d/%d ===",
                 gs->current_turn, gs->config.max_turns);
        draw_text(s_win, s_font, buf, col, row,
                  FONT_SIZE_MD, sfColor_fromRGB(100, 180, 255));
        row += 30;

        if (gs->victory.achieved) {
            if (gs->victory.winner_owner == PLAYER_OWNER_ID) {
                draw_text(s_win, s_font,
                          "*** VICTOIRE ! Vous avez gagne ! ***",
                          col, row, FONT_SIZE_MD,
                          sfColor_fromRGB(255, 215, 0));
            } else {
                snprintf(buf, sizeof(buf),
                         "*** DEFAITE - Joueur #%d a gagne ***",
                         gs->victory.winner_owner);
                draw_text(s_win, s_font, buf, col, row, FONT_SIZE_MD,
                          sfColor_fromRGB(220, 60, 60));
            }
        } else {
            draw_text(s_win, s_font, "Partie terminee (abandon).",
                      col, row, FONT_SIZE_MD,
                      sfColor_fromRGB(180, 180, 180));
        }
        row += 40;

        draw_text(s_win, s_font, "--- Scores finaux ---",
                  col, row, FONT_SIZE_SM,
                  sfColor_fromRGB(180, 180, 180));
        row += 22;

        int p_cities = 0;
        int p_units  = 0;
        for (int i = 0; i < gs->cities.count; i++)
            if (gs->cities.data[i].is_active &&
                gs->cities.data[i].owner == PLAYER_OWNER_ID)
                p_cities++;
        for (int i = 0; i < gs->units.count; i++)
            if (gs->units.data[i].is_active &&
                gs->units.data[i].owner == PLAYER_OWNER_ID)
                p_units++;
        snprintf(buf, sizeof(buf),
                 "  Vous        : %4d pts   %d villes  %d unites  culture:%d",
                 gs->player.score, p_cities, p_units,
                 gs->player.culture_points);
        draw_text(s_win, s_font, buf, col, row, FONT_SIZE_SM,
                  sfColor_fromRGB(255, 215, 0));
        row += 20;

        for (int i = 0; i < gs->ai_factions.count; i++) {
            AIFaction *f = &gs->ai_factions.data[i];
            int fc = 0;
            int fu = 0;
            for (int j = 0; j < gs->cities.count; j++)
                if (gs->cities.data[j].is_active &&
                    gs->cities.data[j].owner == f->id) fc++;
            for (int j = 0; j < gs->units.count; j++)
                if (gs->units.data[j].is_active &&
                    gs->units.data[j].owner == f->id) fu++;
            if (f->is_eliminated)
                snprintf(buf, sizeof(buf), "  %-12s: ELIMINE", f->name);
            else
                snprintf(buf, sizeof(buf),
                         "  %-12s: %4d pts   %d villes  %d unites",
                         f->name, f->score, fc, fu);
            draw_text(s_win, s_font, buf, col, row, FONT_SIZE_SM,
                      sfColor_fromRGB(180, 180, 200));
            row += 20;
        }
        row += 20;
        draw_text(s_win, s_font, "[R] Rejouer     [Q] Quitter",
                  col, row, FONT_SIZE_MD,
                  sfColor_fromRGB(180, 180, 180));

        sfRenderWindow_display(s_win);
    }
    return false;
}
