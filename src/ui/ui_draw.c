#include "ui/ui_draw.h"
#include "ui/ui_layout.h"
#include "tech/tech_tree.h"
#include "entities/unit.h"
#include "entities/city.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void fill_rect(float x, float y, float rw, float rh, Color col)
{
    DrawRectangleRec((Rectangle){x, y, rw, rh}, col);
}

static void draw_text(Font f, const char *str, float x, float y,
                      int sz, Color col)
{
    DrawTextEx(f, str, (Vector2){x, y}, (float)sz, 1.0f, col);
}

static void draw_btn(Font f, float x, float y, float bw, float bh,
                     const char *label, Color bg, Color border)
{
    Rectangle rec = {x, y, bw, bh};

    DrawRectangleRec(rec, bg);
    DrawRectangleLinesEx(rec, 1.0f, border);
    draw_text(f, label, x + 6, y + 6, FONT_SIZE_SM, COL_TEXT);
}

void draw_toolbar(Font f, GameState *gs)
{
    char buf[128];

    fill_rect(0, 0, WIN_W, TOOLBAR_H, COL_TOOLBAR);

    snprintf(buf, sizeof(buf), "Tour %d/%d",
             gs->current_turn, gs->config.max_turns);
    draw_text(f, buf, 10, 12, FONT_SIZE_MD, COL_TITLE);

    snprintf(buf, sizeof(buf), "Or: %d (+%d)",
             gs->player.gold, gs->player.gold_per_turn);
    draw_text(f, buf, 160, 12, FONT_SIZE_SM, COL_PLAYER);

    snprintf(buf, sizeof(buf), "Sci: %d (+%d)",
             gs->player.science, gs->player.science_per_turn);
    draw_text(f, buf, 310, 12, FONT_SIZE_SM, COL_TITLE);

    snprintf(buf, sizeof(buf), "Culture: %d  Score: %d",
             gs->player.culture_points, gs->player.score);
    draw_text(f, buf, 450, 12, FONT_SIZE_SM, COL_TEXT);

    draw_btn(f, BTN_RECH_X, BTN_Y, BTN_RECH_W, BTN_H,
             "Recherche", COL_BTN_STD, COL_BTN_BORDER);
    draw_btn(f, BTN_SAVE_X, BTN_Y, BTN_SAVE_W, BTN_H,
             "Save", COL_BTN_STD, COL_BTN_BORDER);
    draw_btn(f, BTN_LOAD_X, BTN_Y, BTN_LOAD_W, BTN_H,
             "Load", COL_BTN_STD, COL_BTN_BORDER);
    draw_btn(f, BTN_NEXT_X, BTN_Y, BTN_NEXT_W, BTN_H,
             "Tour suivant", COL_BTN_NEXT, COL_BTN_PBORDER);
}

void draw_map(Font f, GameState *gs, int selected_id, int mode)
{
    float tw = (float)MAP_W / gs->map.width;
    float th = (float)MAP_H / gs->map.height;
    char label[4];

    fill_rect(MAP_X, MAP_Y, MAP_W, MAP_H, COL_BG);

    int y = 0;
    while (y < gs->map.height) {
        int x = 0;
        while (x < gs->map.width) {
            Tile *t = &gs->map.grid[y][x];
            Color tc = (t->type == TERRAIN_PLAIN)    ? COL_PLAIN :
                       (t->type == TERRAIN_MOUNTAIN)  ? COL_MOUNTAIN :
                                                        COL_WATER;
            float px = MAP_X + x * tw;
            float py = MAP_Y + y * th;
            Rectangle rec = {px, py, tw - 1, th - 1};

            if (mode == 2 && selected_id != NO_ID) {
                Unit *u = unit_get(gs, selected_id);
                if (u) {
                    int dist = abs(x - u->x) + abs(y - u->y);
                    if (dist > 0 && dist <= u->moves_left &&
                        t->type != TERRAIN_WATER)
                        tc = COL_MOVE_HINT;
                }
            }
            DrawRectangleRec(rec, tc);

            if (selected_id != NO_ID) {
                Unit *u = unit_get(gs, selected_id);
                if (u && u->x == x && u->y == y)
                    DrawRectangleLinesEx(rec, 2.0f, COL_SELECT);
            }

            Color lc = COL_TEXT;
            label[0] = '\0';
            if (t->unit_id != NO_ID) {
                Unit *u = unit_get(gs, t->unit_id);
                if (u) {
                    label[0] = (u->owner == PLAYER_OWNER_ID) ? 'P' : 'E';
                    label[1] = '\0';
                    lc = (u->owner == PLAYER_OWNER_ID) ? COL_PLAYER : COL_ENEMY;
                }
            } else if (t->city_id != NO_ID) {
                City *c = city_get(gs, t->city_id);
                if (c) {
                    label[0] = (c->owner == PLAYER_OWNER_ID) ? '@' : '#';
                    label[1] = '\0';
                    lc = (c->owner == PLAYER_OWNER_ID) ? COL_PLAYER : COL_ENEMY;
                }
            }
            if (label[0])
                draw_text(f, label, px + tw / 4, py + 2, FONT_SIZE_SM, lc);
            x++;
        }
        y++;
    }
}

void draw_panel(Font f, GameState *gs, int selected_id, int mode)
{
    char buf[128];
    int row = PANEL_Y + 8;

    fill_rect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, COL_PANEL_BG);

    draw_text(f, "=== Infos ===", PANEL_X + 10, (float)row,
              FONT_SIZE_MD, COL_TITLE);
    row += 22;

    const TechDef *cur = NULL;
    if (gs->player.research.current_tech_id != NO_ID)
        cur = tech_tree_get(gs->player.research.current_tech_id);
    if (cur)
        snprintf(buf, sizeof(buf), "Recherche: %s", cur->name);
    else
        snprintf(buf, sizeof(buf), "Recherche: aucune");
    draw_text(f, buf, PANEL_X + 10, (float)row, FONT_SIZE_SM, COL_TEXT);
    row += 18;

    const char *rel = "aucune";
    int i = 0;
    while (i < gs->religions.count) {
        if (gs->religions.data[i].founder_owner == PLAYER_OWNER_ID) {
            rel = gs->religions.data[i].name;
            break;
        }
        i++;
    }
    snprintf(buf, sizeof(buf), "Religion: %s", rel);
    draw_text(f, buf, PANEL_X + 10, (float)row, FONT_SIZE_SM, COL_TEXT);
    row += 28;

    if (selected_id != NO_ID && mode != 2 && mode != 3) {
        Unit *u = unit_get(gs, selected_id);
        if (u) {
            const UnitTemplate *tmpl = unit_template_get(u->template_id);
            snprintf(buf, sizeof(buf), "Unite #%d: %s",
                     u->id, tmpl ? tmpl->name : "?");
            draw_text(f, buf, PANEL_X + 10, (float)row,
                      FONT_SIZE_MD, COL_PLAYER);
            row += 20;
            snprintf(buf, sizeof(buf), "HP: %d  Mvt: %d  Pos: (%d,%d)",
                     u->hp, u->moves_left, u->x, u->y);
            draw_text(f, buf, PANEL_X + 10, (float)row,
                      FONT_SIZE_SM, COL_TEXT);
            row += 30;
            draw_btn(f, PBTN_X, PBTN_MOVE_Y, PBTN_W, PBTN_H,
                     "Deplacer", COL_BTN_STD, COL_BTN_BORDER);
            draw_btn(f, PBTN_X, PBTN_ATK_Y, PBTN_W, PBTN_H,
                     "Attaquer", COL_BTN_STD, COL_BTN_BORDER);
            draw_btn(f, PBTN_X, PBTN_FOUND_Y, PBTN_W, PBTN_H,
                     "Fonder ville", COL_BTN_STD, COL_BTN_BORDER);
        }
    } else if (mode == 2) {
        draw_text(f, "Cliquez une destination",
                  PANEL_X + 10, (float)row, FONT_SIZE_SM, COL_MOVE_HINT);
    } else if (mode == 3) {
        draw_text(f, "Cliquez un ennemi",
                  PANEL_X + 10, (float)row, FONT_SIZE_SM, COL_ENEMY);
    }

    int erow = PANEL_Y + 250;
    draw_text(f, "=== Ennemis ===", PANEL_X + 10, (float)erow,
              FONT_SIZE_SM, COL_ENEMY);
    erow += 18;
    bool any = false;
    i = 0;
    while (i < gs->units.count && erow < PANEL_Y + PANEL_H - 20) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner == PLAYER_OWNER_ID) {
            i++;
            continue;
        }
        const UnitTemplate *tm = unit_template_get(u->template_id);
        snprintf(buf, sizeof(buf), "#%d %s (%d,%d) hp:%d",
                 u->id, tm ? tm->name : "?", u->x, u->y, u->hp);
        draw_text(f, buf, PANEL_X + 10, (float)erow, FONT_SIZE_SM, COL_ENEMY);
        erow += 16;
        any = true;
        i++;
    }
    if (!any)
        draw_text(f, "aucun", PANEL_X + 10, (float)erow,
                  FONT_SIZE_SM, COL_TEXT);
}

void draw_log(Font f, GameState *gs, int info_count,
              const char info_buf[][96])
{
    fill_rect(0, LOG_Y, WIN_W - PANEL_W, LOG_H, COL_LOG_BG);

    if (info_count > 0) {
        char line[512] = {0};
        int pos = 0;
        int i = 0;
        while (i < info_count && pos < 500) {
            if (i > 0) {
                strncat(line, " | ", (size_t)(511 - pos));
                pos += 3;
            }
            strncat(line, info_buf[i], (size_t)(511 - pos));
            pos = (int)strlen(line);
            i++;
        }
        draw_text(f, line, 8, LOG_Y + 12, FONT_SIZE_SM, COL_TITLE);
        return;
    }

    if (gs->events.count == 0) {
        draw_text(f, "Aucun evenement.", 8, LOG_Y + 12,
                  FONT_SIZE_SM, COL_TEXT);
        return;
    }
    char combined[512] = {0};
    int pos = 0;
    int i = gs->events.count - 1;
    while (i >= 0 && pos < 480) {
        GameEvent *ev = &gs->events.data[i];
        if (pos > 0) {
            strncat(combined, " | ", (size_t)(511 - pos));
            pos += 3;
        }
        strncat(combined, ev->msg, (size_t)(511 - pos));
        pos = (int)strlen(combined);
        i--;
    }
    draw_text(f, combined, 8, LOG_Y + 12, FONT_SIZE_SM, COL_TEXT);
}
