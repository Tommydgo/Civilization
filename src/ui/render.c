#include <raylib.h>
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

typedef enum {
    UI_IDLE,
    UI_UNIT_SELECTED,
    UI_MOVE_MODE,
    UI_ATTACK_MODE,
    UI_DIALOG_INPUT,
    UI_COMMAND_READY
} UIMode;

static Font   s_font;
static int    s_active     = 0;
static UIMode s_mode       = UI_IDLE;
static int    s_selected_id = NO_ID;
static char   s_cmd_buf[128] = {0};

#define INFO_LINES_MAX 40
static char s_info_buf[INFO_LINES_MAX][96];
static int  s_info_count = 0;

static GameState *s_gs = NULL;

void render_init(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIN_W, WIN_H, "Civilization");
    if (!IsWindowReady())
        return;
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    s_font = LoadFont("assets/font.ttf");
    if (s_font.texture.id == 0) {
        fprintf(stderr, "render: cannot load assets/font.ttf\n");
        CloseWindow();
        return;
    }
    s_active = 1;
}

void render_cleanup(void)
{
    if (!s_active)
        return;
    UnloadFont(s_font);
    CloseWindow();
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
    if (!s_active || !IsWindowReady())
        return;
    BeginDrawing();
    ClearBackground(COL_BG);
    draw_toolbar(s_font, gs);
    draw_map(s_font, gs, s_selected_id, s_mode);
    draw_panel(s_font, gs, s_selected_id, s_mode);
    draw_log(s_font, gs, s_info_count,
             (const char (*)[96])s_info_buf);
    EndDrawing();
}

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
    int i = 0;
    while (i < gs->units.count) {
        Unit *u = &gs->units.data[i];
        if (u->is_active && u->owner == PLAYER_OWNER_ID
                         && u->x == tx && u->y == ty)
            return u->id;
        i++;
    }
    return NO_ID;
}

static int enemy_at(GameState *gs, int tx, int ty)
{
    int i = 0;
    while (i < gs->units.count) {
        Unit *u = &gs->units.data[i];
        if (u->is_active && u->owner != PLAYER_OWNER_ID
                         && u->x == tx && u->y == ty)
            return u->id;
        i++;
    }
    return NO_ID;
}

static void handle_left_click(GameState *gs, int mx, int my)
{
    if (IN_RECT(mx, my, BTN_NEXT_X, BTN_Y, BTN_NEXT_W, BTN_H)) {
        strncpy(s_cmd_buf, "next", sizeof(s_cmd_buf));
        s_mode = UI_COMMAND_READY;
    } else if (IN_RECT(mx, my, BTN_RECH_X, BTN_Y, BTN_RECH_W, BTN_H)) {
        char tech[64] = {0};
        dialog_text_input(s_font, "Nom de la technologie :", tech, 64);
        if (tech[0]) {
            snprintf(s_cmd_buf, sizeof(s_cmd_buf), "research %s", tech);
            s_mode = UI_COMMAND_READY;
        }
    } else if (IN_RECT(mx, my, BTN_SAVE_X, BTN_Y, BTN_SAVE_W, BTN_H)) {
        char fname[64] = {0};
        dialog_text_input(s_font, "Nom du fichier de sauvegarde :",
                          fname, 64);
        if (fname[0]) {
            snprintf(s_cmd_buf, sizeof(s_cmd_buf), "save %s", fname);
            s_mode = UI_COMMAND_READY;
        }
    } else if (IN_RECT(mx, my, BTN_LOAD_X, BTN_Y, BTN_LOAD_W, BTN_H)) {
        char fname[64] = {0};
        dialog_text_input(s_font, "Nom du fichier a charger :", fname, 64);
        if (fname[0]) {
            snprintf(s_cmd_buf, sizeof(s_cmd_buf), "load %s", fname);
            s_mode = UI_COMMAND_READY;
        }
    } else if (mx >= PANEL_X && s_mode == UI_UNIT_SELECTED) {
        if (IN_RECT(mx, my, PBTN_X, PBTN_MOVE_Y, PBTN_W, PBTN_H)) {
            s_mode = UI_MOVE_MODE;
        } else if (IN_RECT(mx, my, PBTN_X, PBTN_ATK_Y, PBTN_W, PBTN_H)) {
            s_mode = UI_ATTACK_MODE;
        } else if (IN_RECT(mx, my, PBTN_X, PBTN_FOUND_Y, PBTN_W, PBTN_H)) {
            char cname[64] = {0};
            dialog_text_input(s_font, "Nom de la ville :", cname, 64);
            if (cname[0]) {
                snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                         "create_city %s", cname);
                s_mode = UI_COMMAND_READY;
            }
        }
    } else {
        int tx, ty;
        if (screen_to_tile(gs, mx, my, &tx, &ty)) {
            if (s_mode == UI_MOVE_MODE) {
                snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                         "move %d %d %d", s_selected_id, tx, ty);
                s_mode = UI_COMMAND_READY;
                s_selected_id = NO_ID;
            } else if (s_mode == UI_ATTACK_MODE) {
                int eid = enemy_at(gs, tx, ty);
                if (eid != NO_ID) {
                    snprintf(s_cmd_buf, sizeof(s_cmd_buf),
                             "attack %d %d", s_selected_id, eid);
                    s_mode = UI_COMMAND_READY;
                    s_selected_id = NO_ID;
                }
            } else {
                int uid = unit_at(gs, tx, ty);
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

void render_read_input(char *buf, int len)
{
    if (!s_active || !s_gs) {
        buf[0] = '\0';
        return;
    }

    while (!WindowShouldClose()) {
        render_full(s_gs);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            handle_left_click(s_gs, GetMouseX(), GetMouseY());

        if (IsKeyPressed(KEY_ESCAPE)) {
            s_selected_id = NO_ID;
            s_mode = UI_IDLE;
        }

        if (WindowShouldClose()) {
            strncpy(s_cmd_buf, "quit", sizeof(s_cmd_buf));
            s_mode = UI_COMMAND_READY;
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
    if (!s_active)
        return 0;

    const char *diff[] = {
        "Peaceful  - 0 IA  (exploration libre)",
        "Easy      - 1 IA  (aggression faible)",
        "Medium    - 3 IA  (aggression moderee)",
        "Hard      - 5 IA  (+ tech de depart)",
        "Extreme   - 7 IA  (+ tech + aggression max)"
    };
    int diff_ai[] = {0, 1, 3, 5, 7};
    int sel = dialog_list_select(s_font, "Choisissez la difficulte",
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
    sel = dialog_list_select(s_font, "Choisissez votre civilisation",
                             civs, 10, 0);
    if (sel < 0) sel = 0;
    *civ_id_out    = sel;
    config->civ_id = sel;
    return 0;
}

static void draw_end_scores(GameState *gs, float col, float *row)
{
    char buf[256];
    int p_cities = 0;
    int p_units  = 0;
    int i = 0;

    while (i < gs->cities.count) {
        if (gs->cities.data[i].is_active &&
            gs->cities.data[i].owner == PLAYER_OWNER_ID)
            p_cities++;
        i++;
    }
    i = 0;
    while (i < gs->units.count) {
        if (gs->units.data[i].is_active &&
            gs->units.data[i].owner == PLAYER_OWNER_ID)
            p_units++;
        i++;
    }
    snprintf(buf, sizeof(buf),
             "  Vous        : %4d pts   %d villes  %d unites  culture:%d",
             gs->player.score, p_cities, p_units,
             gs->player.culture_points);
    DrawTextEx(s_font, buf, (Vector2){col, *row}, FONT_SIZE_SM, 1.0f,
               (Color){255, 215, 0, 255});
    *row += 20;

    i = 0;
    while (i < gs->ai_factions.count) {
        AIFaction *f = &gs->ai_factions.data[i];
        int fc = 0;
        int fu = 0;
        int j = 0;
        while (j < gs->cities.count) {
            if (gs->cities.data[j].is_active &&
                gs->cities.data[j].owner == f->id) fc++;
            j++;
        }
        j = 0;
        while (j < gs->units.count) {
            if (gs->units.data[j].is_active &&
                gs->units.data[j].owner == f->id) fu++;
            j++;
        }
        if (f->is_eliminated)
            snprintf(buf, sizeof(buf), "  %-12s: ELIMINE", f->name);
        else
            snprintf(buf, sizeof(buf),
                     "  %-12s: %4d pts   %d villes  %d unites",
                     f->name, f->score, fc, fu);
        DrawTextEx(s_font, buf, (Vector2){col, *row}, FONT_SIZE_SM, 1.0f,
                   (Color){180, 180, 200, 255});
        *row += 20;
        i++;
    }
}

bool render_end_screen(GameState *gs)
{
    if (!s_active)
        return false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q))
            return false;
        if (IsKeyPressed(KEY_R))
            return true;

        BeginDrawing();
        ClearBackground((Color){10, 10, 20, 255});

        char buf[256];
        float col = WIN_W / 5.f;
        float row = 40;

        snprintf(buf, sizeof(buf), "=== PARTIE TERMINEE - Tour %d/%d ===",
                 gs->current_turn, gs->config.max_turns);
        DrawTextEx(s_font, buf, (Vector2){col, row}, FONT_SIZE_MD, 1.0f,
                   (Color){100, 180, 255, 255});
        row += 30;

        if (gs->victory.achieved) {
            if (gs->victory.winner_owner == PLAYER_OWNER_ID) {
                DrawTextEx(s_font, "*** VICTOIRE ! Vous avez gagne ! ***",
                           (Vector2){col, row}, FONT_SIZE_MD, 1.0f,
                           (Color){255, 215, 0, 255});
            } else {
                snprintf(buf, sizeof(buf),
                         "*** DEFAITE - Joueur #%d a gagne ***",
                         gs->victory.winner_owner);
                DrawTextEx(s_font, buf, (Vector2){col, row},
                           FONT_SIZE_MD, 1.0f, (Color){220, 60, 60, 255});
            }
        } else {
            DrawTextEx(s_font, "Partie terminee (abandon).",
                       (Vector2){col, row}, FONT_SIZE_MD, 1.0f,
                       (Color){180, 180, 180, 255});
        }
        row += 40;

        DrawTextEx(s_font, "--- Scores finaux ---",
                   (Vector2){col, row}, FONT_SIZE_SM, 1.0f,
                   (Color){180, 180, 180, 255});
        row += 22;
        draw_end_scores(gs, col, &row);
        row += 20;
        DrawTextEx(s_font, "[R] Rejouer     [Q] Quitter",
                   (Vector2){col, row}, FONT_SIZE_MD, 1.0f,
                   (Color){180, 180, 180, 255});
        EndDrawing();
    }
    return false;
}
