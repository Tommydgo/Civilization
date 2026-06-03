# Migration ncurses → CSFML Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remplacer le module UI ncurses (`src/ui/render.c`) par une interface CSFML HUD avec carte en tuiles, panneau latéral avec boutons, et machine à états gérant les clics souris.

**Architecture:** La façade publique `render.h` reste identique — `game.c`, `command.c` et les tests ne changent pas. `render_read_input` contient la boucle CSFML et une machine à états qui traduit clics souris → commandes texte. Le dessin est séparé dans `ui_draw.c` et les popups dans `ui_dialog.c`.

**Tech Stack:** CSFML 2.6.1 (Nix store), DejaVu Sans Mono (font locale), C99.

---

## Fichiers touchés

| Action | Fichier |
|--------|---------|
| Modifier | `Makefile` |
| Créer | `assets/font.ttf` |
| Créer | `src/ui/ui_layout.h` |
| Remplacer | `src/ui/render.c` |
| Créer | `src/ui/ui_draw.h` |
| Créer | `src/ui/ui_draw.c` |
| Créer | `src/ui/ui_dialog.h` |
| Créer | `src/ui/ui_dialog.c` |
| Inchangé | `src/ui/render.h` |
| Inchangé | `tests/render_stub.c` |

---

## Task 1: Makefile + asset font

**Files:**
- Modify: `Makefile`
- Create: `assets/font.ttf`

- [ ] **Step 1 : Copier la police DejaVu dans assets/**

```bash
mkdir -p assets
cp /nix/store/b9qlzmzj6yqzvxvvh4xpavizi93x1kp3-X11-fonts/share/X11/fonts/DejaVuSansMono.ttf assets/font.ttf
```

- [ ] **Step 2 : Mettre à jour le Makefile**

Remplacer toute la partie détection ncurses/LDFLAGS et la ligne TEST_GAME_SRC par :

```makefile
NAME = civilization
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Isrc

# Locate CSFML 2.x in the Nix store
CSFML_INC := $(shell find /nix/store -maxdepth 6 -name "Graphics.h" -path "*/SFML/*" 2>/dev/null | grep "csfml-2" | head -1 | xargs dirname 2>/dev/null | xargs dirname 2>/dev/null)
CSFML_LIB := $(shell find /nix/store -maxdepth 6 -name "libcsfml-graphics.so" 2>/dev/null | grep "csfml-2" | head -1 | xargs dirname 2>/dev/null)

ifneq ($(CSFML_INC),)
CFLAGS += -I$(CSFML_INC)
endif

LDFLAGS :=
ifneq ($(CSFML_LIB),)
LDFLAGS += -L$(CSFML_LIB) -Wl,-rpath,$(CSFML_LIB)
endif
LDFLAGS += -lcsfml-graphics -lcsfml-window -lcsfml-system

SRC     = $(shell find src -name '*.c')
OBJ     = $(SRC:.c=.o)

# Tests: exclude main + all UI files (render_stub.c replaces them)
TEST_GAME_SRC = $(filter-out src/main.c src/ui/render.c src/ui/ui_draw.c src/ui/ui_dialog.c, $(SRC))
TEST_GAME_OBJ = $(TEST_GAME_SRC:.c=.o)
TEST_SRC      = $(shell find tests -name '*.c')
TEST_OBJ      = $(TEST_SRC:.c=.o)
TEST_BIN      = test_runner

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME) $(LDFLAGS)

tests: $(TEST_GAME_OBJ) $(TEST_OBJ)
	$(CC) $(TEST_GAME_OBJ) $(TEST_OBJ) -o $(TEST_BIN) -Iinclude -Isrc
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TEST_BIN)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re tests
```

- [ ] **Step 3 : Vérifier que les tests passent toujours**

```bash
make tests
```

Expected : tous les tests passent (render_stub.c fournit les stubs).

- [ ] **Step 4 : Commit**

```bash
git add Makefile assets/font.ttf
git commit -m "build: switch to CSFML, add font asset"
```

---

## Task 2: `src/ui/ui_layout.h` — constantes de layout partagées

**Files:**
- Create: `src/ui/ui_layout.h`

- [ ] **Step 1 : Créer le fichier**

```c
#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

/* Window */
#define WIN_W       1280
#define WIN_H       720

/* Zones */
#define TOOLBAR_H   40
#define LOG_H       40
#define MAP_X       0
#define MAP_W       880
#define MAP_Y       TOOLBAR_H
#define MAP_H       (WIN_H - TOOLBAR_H - LOG_H)
#define PANEL_X     MAP_W
#define PANEL_W     (WIN_W - MAP_W)
#define PANEL_Y     TOOLBAR_H
#define PANEL_H     MAP_H
#define LOG_Y       (WIN_H - LOG_H)

/* Toolbar buttons (right-aligned, y=5, h=30) */
#define BTN_NEXT_X      (WIN_W - 150)
#define BTN_NEXT_W      140
#define BTN_LOAD_X      (BTN_NEXT_X - 100)
#define BTN_LOAD_W      90
#define BTN_SAVE_X      (BTN_LOAD_X - 100)
#define BTN_SAVE_W      90
#define BTN_RECH_X      (BTN_SAVE_X - 120)
#define BTN_RECH_W      110
#define BTN_Y           5
#define BTN_H           30

/* Panel action buttons (shown when unit selected) */
#define PBTN_X          (PANEL_X + 10)
#define PBTN_W          (PANEL_W - 20)
#define PBTN_H          28
#define PBTN_MOVE_Y     (PANEL_Y + 120)
#define PBTN_ATK_Y      (PANEL_Y + 155)
#define PBTN_FOUND_Y    (PANEL_Y + 190)

/* Colors */
#define COL_BG          sfColor_fromRGB(15,  15,  26)
#define COL_TOOLBAR     sfColor_fromRGB(26,  26,  46)
#define COL_PANEL_BG    sfColor_fromRGB(10,  10,  20)
#define COL_LOG_BG      sfColor_fromRGB(10,  10,  20)
#define COL_PLAIN       sfColor_fromRGB(45,  90,  45)
#define COL_MOUNTAIN    sfColor_fromRGB(80,  80,  80)
#define COL_WATER       sfColor_fromRGB(30,  60, 140)
#define COL_PLAYER      sfColor_fromRGB(255,215,   0)
#define COL_ENEMY       sfColor_fromRGB(220, 60,  60)
#define COL_SELECT      sfColor_fromRGB(255,215,   0)
#define COL_MOVE_HINT   sfColor_fromRGB(100,200, 100)
#define COL_TEXT        sfColor_fromRGB(200,200,200)
#define COL_WHITE       sfColor_fromRGB(255,255,255)
#define COL_TITLE       sfColor_fromRGB(100,180,255)
#define COL_BTN_NEXT    sfColor_fromRGB(30,  80,  30)
#define COL_BTN_STD     sfColor_fromRGB(30,  30,  60)
#define COL_BTN_BORDER  sfColor_fromRGB(80,  80, 150)
#define COL_BTN_PBORDER sfColor_fromRGB(80, 150,  80)

/* Font size */
#define FONT_SIZE_SM    12
#define FONT_SIZE_MD    14

/* Helper: point-in-rect test */
#define IN_RECT(px,py,rx,ry,rw,rh) \
    ((px)>=(rx) && (px)<(rx)+(rw) && (py)>=(ry) && (py)<(ry)+(rh))

#endif
```

- [ ] **Step 2 : Vérifier que les tests compilent toujours**

```bash
make tests
```

- [ ] **Step 3 : Commit**

```bash
git add src/ui/ui_layout.h
git commit -m "ui: add layout constants header"
```

---

## Task 3: `render.c` — squelette CSFML (window + state + API publique)

**Files:**
- Replace: `src/ui/render.c`

- [ ] **Step 1 : Écrire le nouveau render.c**

```c
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
   implemented in Tasks 10, 12 */
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
```

- [ ] **Step 2 : Créer des stubs temporaires pour ui_draw et ui_dialog**

Créer `src/ui/ui_draw.h` :
```c
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
```

Créer `src/ui/ui_draw.c` (stubs) :
```c
#include "ui/ui_draw.h"
void draw_toolbar(sfRenderWindow *w, sfFont *f, GameState *gs)
    { (void)w;(void)f;(void)gs; }
void draw_map(sfRenderWindow *w, sfFont *f, GameState *gs, int sel, int mode)
    { (void)w;(void)f;(void)gs;(void)sel;(void)mode; }
void draw_panel(sfRenderWindow *w, sfFont *f, GameState *gs, int sel, int mode)
    { (void)w;(void)f;(void)gs;(void)sel;(void)mode; }
void draw_log(sfRenderWindow *w, sfFont *f, GameState *gs, int n, const char b[][96])
    { (void)w;(void)f;(void)gs;(void)n;(void)b; }
```

Créer `src/ui/ui_dialog.h` :
```c
#ifndef UI_DIALOG_H
#define UI_DIALOG_H
#include <SFML/Graphics.h>
void dialog_text_input(sfRenderWindow *win, sfFont *font,
                       const char *prompt, char *buf, int len);
int  dialog_list_select(sfRenderWindow *win, sfFont *font,
                        const char *title, const char **items,
                        int count, int default_sel);
#endif
```

Créer `src/ui/ui_dialog.c` (stubs) :
```c
#include "ui/ui_dialog.h"
#include <string.h>
void dialog_text_input(sfRenderWindow *w, sfFont *f,
                       const char *p, char *buf, int len)
    { (void)w;(void)f;(void)p; strncpy(buf,"",len); }
int dialog_list_select(sfRenderWindow *w, sfFont *f,
                       const char *t, const char **i, int c, int d)
    { (void)w;(void)f;(void)t;(void)i;(void)c; return d; }
```

- [ ] **Step 3 : Compiler le jeu (sans crash)**

```bash
make
```

Expected : compile sans erreur. `./civilization` ouvre une fenêtre noire (rendu vide).

- [ ] **Step 4 : Vérifier les tests**

```bash
make tests
```

Expected : tous les tests passent.

- [ ] **Step 5 : Commit**

```bash
git add src/ui/render.c src/ui/ui_draw.h src/ui/ui_draw.c \
        src/ui/ui_dialog.h src/ui/ui_dialog.c
git commit -m "ui: CSFML skeleton — window opens, stubs compile"
```

---

## Task 4: `ui_draw.c` — draw_toolbar

**Files:**
- Modify: `src/ui/ui_draw.c`

- [ ] **Step 1 : Implémenter deux helpers locaux dans ui_draw.c**

Ajouter en tête de fichier (après les includes) :

```c
#include "ui/ui_draw.h"
#include "ui/ui_layout.h"
#include "tech/tech_tree.h"
#include <stdio.h>
#include <string.h>

/* Draw a filled rectangle */
static void fill_rect(sfRenderWindow *w, float x, float y,
                      float rw, float rh, sfColor col)
{
    sfRectangleShape *r = sfRectangleShape_create();
    sfRectangleShape_setPosition(r, (sfVector2f){x, y});
    sfRectangleShape_setSize(r, (sfVector2f){rw, rh});
    sfRectangleShape_setFillColor(r, col);
    sfRenderWindow_drawRectangleShape(w, r, NULL);
    sfRectangleShape_destroy(r);
}

/* Draw text at (x,y) with given size and color */
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

/* Draw a button: filled rect + centered label */
static void draw_btn(sfRenderWindow *w, sfFont *f, float x, float y,
                     float bw, float bh, const char *label,
                     sfColor bg, sfColor border)
{
    sfRectangleShape *r = sfRectangleShape_create();
    sfRectangleShape_setPosition(r, (sfVector2f){x, y});
    sfRectangleShape_setSize(r, (sfVector2f){bw, bh});
    sfRectangleShape_setFillColor(r, bg);
    sfRectangleShape_setOutlineColor(r, border);
    sfRectangleShape_setOutlineThickness(r, 1.f);
    sfRenderWindow_drawRectangleShape(w, r, NULL);
    sfRectangleShape_destroy(r);
    draw_text(w, f, label, x + 6, y + 6, FONT_SIZE_SM, COL_TEXT);
}
```

- [ ] **Step 2 : Implémenter draw_toolbar**

```c
void draw_toolbar(sfRenderWindow *w, sfFont *f, GameState *gs)
{
    char buf[128];

    fill_rect(w, 0, 0, WIN_W, TOOLBAR_H, COL_TOOLBAR);

    snprintf(buf, sizeof(buf), "Tour %d/%d",
             gs->current_turn, gs->config.max_turns);
    draw_text(w, f, buf, 10, 12, FONT_SIZE_MD, COL_TITLE);

    snprintf(buf, sizeof(buf), "Or: %d (+%d)",
             gs->player.gold, gs->player.gold_per_turn);
    draw_text(w, f, buf, 160, 12, FONT_SIZE_SM, COL_PLAYER);

    snprintf(buf, sizeof(buf), "Sci: %d (+%d)",
             gs->player.science, gs->player.science_per_turn);
    draw_text(w, f, buf, 310, 12, FONT_SIZE_SM, COL_TITLE);

    snprintf(buf, sizeof(buf), "Culture: %d  Score: %d",
             gs->player.culture_points, gs->player.score);
    draw_text(w, f, buf, 450, 12, FONT_SIZE_SM, COL_TEXT);

    draw_btn(w, f, BTN_RECH_X, BTN_Y, BTN_RECH_W, BTN_H,
             "Recherche", COL_BTN_STD, COL_BTN_BORDER);
    draw_btn(w, f, BTN_SAVE_X, BTN_Y, BTN_SAVE_W, BTN_H,
             "Save", COL_BTN_STD, COL_BTN_BORDER);
    draw_btn(w, f, BTN_LOAD_X, BTN_Y, BTN_LOAD_W, BTN_H,
             "Load", COL_BTN_STD, COL_BTN_BORDER);
    draw_btn(w, f, BTN_NEXT_X, BTN_Y, BTN_NEXT_W, BTN_H,
             "Tour suivant", COL_BTN_NEXT, COL_BTN_PBORDER);
}
```

- [ ] **Step 3 : Compiler et vérifier visuellement**

```bash
make && ./civilization
```

Expected : fenêtre avec barre du haut affichant les stats et boutons. Passer la difficulté au plus bas pour tester vite — l'interface est encore vide sinon.

- [ ] **Step 4 : Commit**

```bash
git add src/ui/ui_draw.c
git commit -m "ui: draw_toolbar with status and buttons"
```

---

## Task 5: `ui_draw.c` — draw_map

**Files:**
- Modify: `src/ui/ui_draw.c`

- [ ] **Step 1 : Implémenter draw_map**

```c
void draw_map(sfRenderWindow *w, sfFont *f, GameState *gs,
              int selected_id, int mode)
{
    float tw = (float)MAP_W / gs->map.width;
    float th = (float)MAP_H / gs->map.height;
    char label[4];

    fill_rect(w, MAP_X, MAP_Y, MAP_W, MAP_H, COL_BG);

    for (int y = 0; y < gs->map.height; y++) {
        for (int x = 0; x < gs->map.width; x++) {
            Tile *t = &gs->map.grid[y][x];
            sfColor tc = (t->type == TERRAIN_PLAIN)    ? COL_PLAIN :
                         (t->type == TERRAIN_MOUNTAIN)  ? COL_MOUNTAIN :
                                                          COL_WATER;
            float px = MAP_X + x * tw;
            float py = MAP_Y + y * th;

            /* Tile background */
            sfRectangleShape *r = sfRectangleShape_create();
            sfRectangleShape_setPosition(r, (sfVector2f){px, py});
            sfRectangleShape_setSize(r, (sfVector2f){tw - 1, th - 1});
            sfRectangleShape_setFillColor(r, tc);

            /* Move mode hint: highlight reachable tiles */
            if (mode == UI_MOVE_MODE && selected_id != NO_ID) {
                Unit *u = unit_get(gs, selected_id);
                if (u) {
                    int dist = abs(x - u->x) + abs(y - u->y);
                    if (dist > 0 && dist <= u->moves_left &&
                        t->type != TERRAIN_WATER)
                        sfRectangleShape_setFillColor(r, COL_MOVE_HINT);
                }
            }

            /* Selection outline */
            if (selected_id != NO_ID) {
                Unit *u = unit_get(gs, selected_id);
                if (u && u->x == x && u->y == y) {
                    sfRectangleShape_setOutlineColor(r, COL_SELECT);
                    sfRectangleShape_setOutlineThickness(r, 2.f);
                }
            }
            sfRenderWindow_drawRectangleShape(w, r, NULL);
            sfRectangleShape_destroy(r);

            /* Entity label */
            sfColor lc = COL_TEXT;
            label[0] = '\0';
            if (t->unit_id != NO_ID) {
                Unit *u = unit_get(gs, t->unit_id);
                if (u) {
                    label[0] = (u->owner == PLAYER_OWNER_ID) ? 'P' : 'E';
                    label[1] = '\0';
                    lc = (u->owner == PLAYER_OWNER_ID) ? COL_PLAYER
                                                        : COL_ENEMY;
                }
            } else if (t->city_id != NO_ID) {
                City *c = city_get(gs, t->city_id);
                if (c) {
                    label[0] = (c->owner == PLAYER_OWNER_ID) ? '@' : '#';
                    label[1] = '\0';
                    lc = (c->owner == PLAYER_OWNER_ID) ? COL_PLAYER
                                                        : COL_ENEMY;
                }
            }
            if (label[0])
                draw_text(w, f, label, px + tw / 4, py + 2,
                          FONT_SIZE_SM, lc);
        }
    }
}
```

Ajouter les includes nécessaires en haut de `ui_draw.c` :
```c
#include "entities/unit.h"
#include "entities/city.h"
#include <stdlib.h>   /* abs() */
```

- [ ] **Step 2 : Compiler et vérifier visuellement**

```bash
make && ./civilization
```

Expected : la carte s'affiche avec tuiles colorées (vert/gris/bleu) et labels P, @, E, # pour les entités.

- [ ] **Step 3 : Commit**

```bash
git add src/ui/ui_draw.c
git commit -m "ui: draw_map with terrain tiles and entity labels"
```

---

## Task 6: `ui_draw.c` — draw_panel

**Files:**
- Modify: `src/ui/ui_draw.c`

- [ ] **Step 1 : Implémenter draw_panel**

```c
void draw_panel(sfRenderWindow *w, sfFont *f, GameState *gs,
                int selected_id, int mode)
{
    char buf[128];
    int row = PANEL_Y + 8;

    fill_rect(w, PANEL_X, PANEL_Y, PANEL_W, PANEL_H, COL_PANEL_BG);

    /* Title */
    draw_text(w, f, "=== Infos ===", PANEL_X + 10, row,
              FONT_SIZE_MD, COL_TITLE);
    row += 22;

    /* Player stats */
    const TechDef *cur = NULL;
    if (gs->player.research.current_tech_id != NO_ID)
        cur = tech_tree_get(gs->player.research.current_tech_id);
    if (cur)
        snprintf(buf, sizeof(buf), "Recherche: %s", cur->name);
    else
        snprintf(buf, sizeof(buf), "Recherche: aucune");
    draw_text(w, f, buf, PANEL_X + 10, row, FONT_SIZE_SM, COL_TEXT);
    row += 18;

    const char *rel = "aucune";
    for (int i = 0; i < gs->religions.count; i++) {
        if (gs->religions.data[i].founder_owner == PLAYER_OWNER_ID) {
            rel = gs->religions.data[i].name;
            break;
        }
    }
    snprintf(buf, sizeof(buf), "Religion: %s", rel);
    draw_text(w, f, buf, PANEL_X + 10, row, FONT_SIZE_SM, COL_TEXT);
    row += 28;

    /* Selected unit info + action buttons */
    if (selected_id != NO_ID && mode != UI_MOVE_MODE
                              && mode != UI_ATTACK_MODE) {
        Unit *u = unit_get(gs, selected_id);
        if (u) {
            const UnitTemplate *tmpl = unit_template_get(u->template_id);
            snprintf(buf, sizeof(buf), "Unite #%d: %s",
                     u->id, tmpl ? tmpl->name : "?");
            draw_text(w, f, buf, PANEL_X+10, row, FONT_SIZE_MD, COL_PLAYER);
            row += 20;
            snprintf(buf, sizeof(buf), "HP: %d  Mvt: %d  Pos: (%d,%d)",
                     u->hp, u->moves_left, u->x, u->y);
            draw_text(w, f, buf, PANEL_X+10, row, FONT_SIZE_SM, COL_TEXT);
            row += 30;
            draw_btn(w, f, PBTN_X, PBTN_MOVE_Y, PBTN_W, PBTN_H,
                     "Deplacer", COL_BTN_STD, COL_BTN_BORDER);
            draw_btn(w, f, PBTN_X, PBTN_ATK_Y, PBTN_W, PBTN_H,
                     "Attaquer", COL_BTN_STD, COL_BTN_BORDER);
            draw_btn(w, f, PBTN_X, PBTN_FOUND_Y, PBTN_W, PBTN_H,
                     "Fonder ville", COL_BTN_STD, COL_BTN_BORDER);
        }
    } else if (mode == UI_MOVE_MODE) {
        draw_text(w, f, "Cliquez une destination",
                  PANEL_X + 10, row, FONT_SIZE_SM, COL_MOVE_HINT);
    } else if (mode == UI_ATTACK_MODE) {
        draw_text(w, f, "Cliquez un ennemi",
                  PANEL_X + 10, row, FONT_SIZE_SM, COL_ENEMY);
    }

    /* Enemy list */
    int erow = PANEL_Y + 250;
    draw_text(w, f, "=== Ennemis ===", PANEL_X+10, erow,
              FONT_SIZE_SM, COL_ENEMY);
    erow += 18;
    bool any = false;
    for (int i = 0; i < gs->units.count && erow < PANEL_Y+PANEL_H-20; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner == PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tm = unit_template_get(u->template_id);
        snprintf(buf, sizeof(buf), "#%d %s (%d,%d) hp:%d",
                 u->id, tm ? tm->name : "?", u->x, u->y, u->hp);
        draw_text(w, f, buf, PANEL_X+10, erow, FONT_SIZE_SM, COL_ENEMY);
        erow += 16;
        any = true;
    }
    if (!any)
        draw_text(w, f, "aucun", PANEL_X+10, erow, FONT_SIZE_SM, COL_TEXT);
}
```

Ajouter en haut de `ui_draw.c` :
```c
#include "tech/tech_tree.h"
#include "empire/religion.h"   /* si nécessaire pour gs->religions */
```

Note : si `religion.h` n'existe pas comme header séparé, `gs->religions` est défini dans `structs.h` — pas besoin d'include supplémentaire.

- [ ] **Step 2 : Compiler et tester visuellement**

```bash
make && ./civilization
```

Expected : panneau droit affiche recherche en cours, religion, et liste des ennemis.

- [ ] **Step 3 : Commit**

```bash
git add src/ui/ui_draw.c
git commit -m "ui: draw_panel with stats, action buttons, enemy list"
```

---

## Task 7: `ui_draw.c` — draw_log

**Files:**
- Modify: `src/ui/ui_draw.c`

- [ ] **Step 1 : Implémenter draw_log**

```c
void draw_log(sfRenderWindow *w, sfFont *f, GameState *gs,
              int info_count, const char info_buf[][96])
{
    fill_rect(w, 0, LOG_Y, WIN_W - PANEL_W, LOG_H, COL_LOG_BG);

    int avail = (WIN_W - PANEL_W - 10) / 8; /* approx chars visible */
    (void)avail;

    if (info_count > 0) {
        /* Show info buffer (used by build/tech/help commands) */
        /* Concatenate all lines separated by " | " */
        char line[512] = {0};
        int pos = 0;
        for (int i = 0; i < info_count && pos < 500; i++) {
            if (i > 0) { strncat(line, " | ", 511 - pos); pos += 3; }
            strncat(line, info_buf[i], 511 - pos);
            pos = (int)strlen(line);
        }
        draw_text(w, f, line, 8, LOG_Y + 12, FONT_SIZE_SM, COL_TITLE);
        return;
    }

    /* Show last N game events */
    if (gs->events.count == 0) {
        draw_text(w, f, "Aucun evenement.", 8, LOG_Y + 12,
                  FONT_SIZE_SM, COL_TEXT);
        return;
    }
    int start = gs->events.count - 1;
    char combined[512] = {0};
    int pos = 0;
    for (int i = start; i >= 0 && pos < 480; i--) {
        GameEvent *ev = &gs->events.data[i];
        if (pos > 0) { strncat(combined, " | ", 511-pos); pos += 3; }
        strncat(combined, ev->msg, 511 - pos);
        pos = (int)strlen(combined);
    }
    draw_text(w, f, combined, 8, LOG_Y + 12, FONT_SIZE_SM, COL_TEXT);
}
```

- [ ] **Step 2 : Compiler et tester**

```bash
make && ./civilization
```

Expected : barre du bas affiche les événements récents.

- [ ] **Step 3 : Commit**

```bash
git add src/ui/ui_draw.c
git commit -m "ui: draw_log with event feed and info buffer"
```

---

## Task 8: `ui_dialog.c` — dialog_text_input

**Files:**
- Replace: `src/ui/ui_dialog.c`

- [ ] **Step 1 : Implémenter dialog_text_input**

```c
#include "ui/ui_dialog.h"
#include "ui/ui_layout.h"
#include <SFML/Graphics.h>
#include <string.h>
#include <stdio.h>

static void fill_rect_d(sfRenderWindow *w, float x, float y,
                        float rw, float rh, sfColor col)
{
    sfRectangleShape *r = sfRectangleShape_create();
    sfRectangleShape_setPosition(r, (sfVector2f){x, y});
    sfRectangleShape_setSize(r, (sfVector2f){rw, rh});
    sfRectangleShape_setFillColor(r, col);
    sfRenderWindow_drawRectangleShape(w, r, NULL);
    sfRectangleShape_destroy(r);
}

static void draw_text_d(sfRenderWindow *w, sfFont *f, const char *str,
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

void dialog_text_input(sfRenderWindow *win, sfFont *font,
                       const char *prompt, char *buf, int len)
{
    char input[128] = {0};
    int  cursor = 0;
    sfEvent ev;

    while (sfRenderWindow_isOpen(win)) {
        while (sfRenderWindow_pollEvent(win, &ev)) {
            if (ev.type == sfEvtClosed) {
                strncpy(buf, "", (size_t)len);
                return;
            }
            if (ev.type == sfEvtKeyPressed) {
                if (ev.key.code == sfKeyEnter) {
                    strncpy(buf, input, (size_t)(len - 1));
                    buf[len - 1] = '\0';
                    return;
                }
                if (ev.key.code == sfKeyEscape) {
                    strncpy(buf, "", (size_t)len);
                    return;
                }
                if (ev.key.code == sfKeyBackspace && cursor > 0) {
                    input[--cursor] = '\0';
                }
            }
            if (ev.type == sfEvtTextEntered) {
                sfUint32 c = ev.text.unicode;
                if (c >= 32 && c < 127 && cursor < len - 2) {
                    input[cursor++] = (char)c;
                    input[cursor]   = '\0';
                }
            }
        }

        /* Draw overlay */
        float pw = 500, ph = 120;
        float px = (WIN_W - pw) / 2;
        float py = (WIN_H - ph) / 2;

        /* Dim background */
        fill_rect_d(win, 0, 0, WIN_W, WIN_H,
                    sfColor_fromRGBA(0, 0, 0, 160));
        /* Popup box */
        fill_rect_d(win, px, py, pw, ph,
                    sfColor_fromRGB(26, 26, 46));
        /* Border */
        sfRectangleShape *border = sfRectangleShape_create();
        sfRectangleShape_setPosition(border, (sfVector2f){px, py});
        sfRectangleShape_setSize(border, (sfVector2f){pw, ph});
        sfRectangleShape_setFillColor(border, sfColor_fromRGBA(0,0,0,0));
        sfRectangleShape_setOutlineColor(border,
                                         sfColor_fromRGB(74, 74, 200));
        sfRectangleShape_setOutlineThickness(border, 1.f);
        sfRenderWindow_drawRectangleShape(win, border, NULL);
        sfRectangleShape_destroy(border);

        draw_text_d(win, font, prompt, px + 12, py + 14,
                    FONT_SIZE_MD, sfColor_fromRGB(255, 215, 0));

        /* Input field */
        fill_rect_d(win, px + 12, py + 50, pw - 24, 30,
                    sfColor_fromRGB(10, 10, 30));
        /* Input text + cursor */
        char display[130];
        snprintf(display, sizeof(display), "%s_", input);
        draw_text_d(win, font, display, px + 16, py + 56,
                    FONT_SIZE_MD, sfColor_fromRGB(255, 255, 255));

        draw_text_d(win, font, "[Entree] confirmer   [Echap] annuler",
                    px + 12, py + 92, FONT_SIZE_SM,
                    sfColor_fromRGB(120, 120, 140));

        sfRenderWindow_display(win);
    }
    strncpy(buf, "", (size_t)len);
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

Expected : compile sans erreur.

- [ ] **Step 3 : Commit**

```bash
git add src/ui/ui_dialog.c
git commit -m "ui: dialog_text_input modal popup"
```

---

## Task 9: `ui_dialog.c` — dialog_list_select

**Files:**
- Modify: `src/ui/ui_dialog.c`

- [ ] **Step 1 : Implémenter dialog_list_select**

```c
int dialog_list_select(sfRenderWindow *win, sfFont *font,
                       const char *title, const char **items,
                       int count, int default_sel)
{
    int sel = default_sel;
    sfEvent ev;

    while (sfRenderWindow_isOpen(win)) {
        while (sfRenderWindow_pollEvent(win, &ev)) {
            if (ev.type == sfEvtClosed)
                return -1;
            if (ev.type == sfEvtKeyPressed) {
                if (ev.key.code == sfKeyUp   && sel > 0)         sel--;
                if (ev.key.code == sfKeyDown && sel < count - 1) sel++;
                if (ev.key.code == sfKeyEnter)  return sel;
                if (ev.key.code == sfKeyEscape) return default_sel;
            }
        }

        float pw = 600;
        float ph = 60 + count * 22 + 30;
        if (ph > WIN_H - 40) ph = (float)(WIN_H - 40);
        float px = (WIN_W - pw) / 2;
        float py = (WIN_H - ph) / 2;

        fill_rect_d(win, 0, 0, WIN_W, WIN_H,
                    sfColor_fromRGBA(0, 0, 0, 160));
        fill_rect_d(win, px, py, pw, ph,
                    sfColor_fromRGB(20, 20, 40));

        sfRectangleShape *border = sfRectangleShape_create();
        sfRectangleShape_setPosition(border, (sfVector2f){px, py});
        sfRectangleShape_setSize(border, (sfVector2f){pw, ph});
        sfRectangleShape_setFillColor(border, sfColor_fromRGBA(0,0,0,0));
        sfRectangleShape_setOutlineColor(border,
                                         sfColor_fromRGB(74, 74, 200));
        sfRectangleShape_setOutlineThickness(border, 1.f);
        sfRenderWindow_drawRectangleShape(win, border, NULL);
        sfRectangleShape_destroy(border);

        draw_text_d(win, font, title, px + 12, py + 10,
                    FONT_SIZE_MD, sfColor_fromRGB(255, 215, 0));
        draw_text_d(win, font,
                    "Haut/Bas pour naviguer, Entree pour confirmer",
                    px + 12, py + 32,
                    FONT_SIZE_SM, sfColor_fromRGB(120, 120, 140));

        for (int i = 0; i < count; i++) {
            float iy = py + 58 + i * 22;
            if (iy + 22 > py + ph - 4) break;
            if (i == sel) {
                fill_rect_d(win, px + 8, iy - 2, pw - 16, 20,
                            sfColor_fromRGB(40, 40, 100));
            }
            sfColor ic = (i == sel) ? sfColor_fromRGB(255, 255, 255)
                                     : sfColor_fromRGB(180, 180, 200);
            draw_text_d(win, font, items[i], px + 14, iy,
                        FONT_SIZE_SM, ic);
        }
        sfRenderWindow_display(win);
    }
    return default_sel;
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

- [ ] **Step 3 : Commit**

```bash
git add src/ui/ui_dialog.c
git commit -m "ui: dialog_list_select scrollable popup"
```

---

## Task 10: `render.c` — machine à états + render_read_input

**Files:**
- Modify: `src/ui/render.c`

- [ ] **Step 1 : Ajouter les includes manquants en haut de render.c**

```c
#include "entities/unit.h"
#include "entities/city.h"
#include "tech/tech_tree.h"
```

- [ ] **Step 2 : Ajouter les helpers de hit-test et de sélection de tuile**

Ajouter ces fonctions statiques avant `render_read_input` dans `render.c` :

```c
/* Convert screen (px, py) to map tile (tx, ty). Returns false if outside map. */
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

/* Find player unit at tile (tx, ty). Returns NO_ID if none. */
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

/* Find any enemy unit at tile (tx, ty). Returns NO_ID if none. */
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
```

- [ ] **Step 3 : Remplacer le stub render_read_input par l'implémentation complète**

```c
void render_read_input(char *buf, int len)
{
    if (!s_active || !s_win || !s_gs) {
        buf[0] = '\0';
        return;
    }

    sfEvent ev;
    while (sfRenderWindow_isOpen(s_win)) {
        /* Re-render each frame so the map stays live */
        render_full(s_gs);

        while (sfRenderWindow_pollEvent(s_win, &ev)) {

            /* Window close → quit */
            if (ev.type == sfEvtClosed) {
                strncpy(s_cmd_buf, "quit", sizeof(s_cmd_buf));
                s_mode = UI_COMMAND_READY;
            }

            if (ev.type == sfEvtMouseButtonPressed &&
                ev.mouseButton.button == sfMouseLeft) {
                int mx = ev.mouseButton.x;
                int my = ev.mouseButton.y;

                /* ── Toolbar buttons ── */
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

                /* ── Panel action buttons (only when unit selected, right zone) ── */
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

                /* ── Map click (works in all non-toolbar states) ── */
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
                            /* IDLE or UNIT_SELECTED: select/deselect */
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

            /* Escape → cancel selection */
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
```

Note : `UI_UNIT_SELECTED`, `UI_MOVE_MODE`, etc. sont les valeurs de l'enum `UIMode` défini dans `render.c`. Le `int mode` dans les signatures de `ui_draw.c` passe ces valeurs par cast implicite — c'est correct en C99.

- [ ] **Step 4 : Compiler et tester**

```bash
make && ./civilization
```

Expected : cliquer une unité `P` sur la carte → boutons apparaissent dans le panneau. Clic "Deplacer" → tuiles vertes. Clic sur une tuile verte → l'unité bouge. Bouton "Tour suivant" → le tour avance.

- [ ] **Step 5 : Vérifier les tests**

```bash
make tests
```

- [ ] **Step 6 : Commit**

```bash
git add src/ui/render.c
git commit -m "ui: render_read_input with full state machine and mouse events"
```

---

## Task 11: `render.c` — render_start_menu + render_end_screen

**Files:**
- Modify: `src/ui/render.c`

- [ ] **Step 1 : Remplacer le stub render_start_menu**

```c
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
    config->difficulty         = sel;
    config->num_ai_factions    = diff_ai[sel];

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
    *civ_id_out   = sel;
    config->civ_id = sel;
    return 0;
}
```

- [ ] **Step 2 : Remplacer le stub render_end_screen**

```c
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
                draw_text(s_win, s_font, "*** VICTOIRE ! Vous avez gagne ! ***",
                          col, row, FONT_SIZE_MD,
                          sfColor_fromRGB(255, 215, 0));
            } else {
                snprintf(buf, sizeof(buf), "*** DEFAITE - Joueur #%d a gagne ***",
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

        int p_cities = 0, p_units = 0;
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
            int fc = 0, fu = 0;
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
```

Note : ajouter cette fonction statique en haut de `render.c` (après les includes de Task 3) pour que `render_end_screen` puisse l'utiliser — c'est un doublon de celle dans `ui_draw.c`, mais les deux fichiers sont compilés séparément :

```c
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
```

- [ ] **Step 3 : Compiler et tester le menu de démarrage**

```bash
make && ./civilization
```

Expected : au lancement → popup de difficulté → popup de civ → partie démarre. À la fin → écran de scores avec [R] / [Q].

- [ ] **Step 4 : Vérifier les tests**

```bash
make tests
```

- [ ] **Step 5 : Commit**

```bash
git add src/ui/render.c
git commit -m "ui: render_start_menu and render_end_screen in CSFML"
```

---

## Task 12: Vérification finale et nettoyage

**Files:**
- Verify: all

- [ ] **Step 1 : Build propre**

```bash
make fclean && make
```

Expected : compile sans warnings `-Wall -Wextra`.

- [ ] **Step 2 : Tests**

```bash
make tests
```

Expected : tous les suites passent (command, city, victory, unit).

- [ ] **Step 3 : Smoke test — partie complète**

```bash
./civilization
```

Vérifier les scénarios suivants :
1. Menu de démarrage → difficulté Medium → Rome → partie lance
2. Clic sur unité Settler → boutons Déplacer/Fonder ville apparaissent
3. Clic Déplacer → tuiles vertes → clic destination → unité bouge
4. Bouton "Tour suivant" → tour +1 dans la toolbar
5. Bouton "Recherche" → popup → taper "Agriculture" → recherche lance
6. Bouton "Save" → popup → taper "test" → fichier sauvegardé
7. Bouton "Load" → popup → taper "test" → partie chargée
8. Fermer la fenêtre → jeu quitte proprement

- [ ] **Step 4 : Commit final**

```bash
git add -A
git commit -m "feat: complete ncurses → CSFML migration with HUD interface"
```
