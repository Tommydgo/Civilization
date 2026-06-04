#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <raylib.h>

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
#define COL_BG          (Color){15,  15,  26,  255}
#define COL_TOOLBAR     (Color){26,  26,  46,  255}
#define COL_PANEL_BG    (Color){10,  10,  20,  255}
#define COL_LOG_BG      (Color){10,  10,  20,  255}
#define COL_PLAIN       (Color){45,  90,  45,  255}
#define COL_MOUNTAIN    (Color){80,  80,  80,  255}
#define COL_WATER       (Color){30,  60, 140,  255}
#define COL_PLAYER      (Color){255, 215,   0, 255}
#define COL_ENEMY       (Color){220,  60,  60, 255}
#define COL_SELECT      (Color){255, 215,   0, 255}
#define COL_MOVE_HINT   (Color){100, 200, 100, 255}
#define COL_TEXT        (Color){200, 200, 200, 255}
#define COL_WHITE       (Color){255, 255, 255, 255}
#define COL_TITLE       (Color){100, 180, 255, 255}
#define COL_BTN_NEXT    (Color){30,   80,  30, 255}
#define COL_BTN_STD     (Color){30,   30,  60, 255}
#define COL_BTN_BORDER  (Color){80,   80, 150, 255}
#define COL_BTN_PBORDER (Color){80,  150,  80, 255}

/* Font sizes */
#define FONT_SIZE_SM    12
#define FONT_SIZE_MD    14

/* Helper: point-in-rect test */
#define IN_RECT(px,py,rx,ry,rw,rh) \
    ((px)>=(rx) && (px)<(rx)+(rw) && (py)>=(ry) && (py)<(ry)+(rh))

#endif
