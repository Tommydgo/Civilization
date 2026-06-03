#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ui/render.h"
#include "events/event.h"
#include "entities/unit.h"
#include "entities/city.h"
#include "tech/tech_tree.h"

// ── Color pair indices ────────────────────────────────────────────────────────
#define CP_PLAIN  1
#define CP_MOUNT  2
#define CP_WATER  3
#define CP_PLAYER 4
#define CP_ENEMY  5
#define CP_TITLE  6

// ── Window layout constants ───────────────────────────────────────────────────
// 2 chars per tile + 3 (row# area) + 2 (borders) + 1 (margin)
#define WIN_HELP_W  32
#define WIN_MAP_W   (MAP_DEFAULT_WIDTH * 2 + 6)  // 106 for 50-wide map (2 chars/tile)

// ── Static windows ────────────────────────────────────────────────────────────
static WINDOW *s_win_help  = NULL;
static WINDOW *s_win_map   = NULL;
static WINDOW *s_win_right = NULL;
static WINDOW *s_win_input = NULL;
static int s_active = 0;

// ── Viewport (camera) ────────────────────────────────────────────────────────
static GameState *s_current_gs = NULL;

// ── Info override buffer (used by 'info' and 'tech' commands) ─────────────────
#define INFO_LINES_MAX 40
static char s_info_buf[INFO_LINES_MAX][96];
static int s_info_count = 0;

// ── Init / cleanup ────────────────────────────────────────────────────────────

static void create_windows(void)
{
    int h = LINES - 3;
    int right_w = COLS - WIN_HELP_W - WIN_MAP_W;

    if (s_win_help)  delwin(s_win_help);
    if (s_win_map)   delwin(s_win_map);
    if (s_win_right) delwin(s_win_right);
    if (s_win_input) delwin(s_win_input);

    s_win_help  = newwin(h, WIN_HELP_W, 0, 0);
    s_win_map   = newwin(h, WIN_MAP_W,  0, WIN_HELP_W);
    s_win_right = newwin(h, right_w,    0, WIN_HELP_W + WIN_MAP_W);
    s_win_input = newwin(3, COLS,       LINES - 3, 0);
}

void render_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    if (has_colors()) {
        start_color();
        init_pair(CP_PLAIN,  COLOR_GREEN,  COLOR_BLACK);
        init_pair(CP_MOUNT,  COLOR_WHITE,  COLOR_BLACK);
        init_pair(CP_WATER,  COLOR_BLUE,   COLOR_BLACK);
        init_pair(CP_PLAYER, COLOR_YELLOW, COLOR_BLACK);
        init_pair(CP_ENEMY,  COLOR_RED,    COLOR_BLACK);
        init_pair(CP_TITLE,  COLOR_CYAN,   COLOR_BLACK);
    }
    create_windows();
    s_active = 1;
}

void render_cleanup(void)
{
    if (!s_active)
        return;
    if (s_win_help)  delwin(s_win_help);
    if (s_win_map)   delwin(s_win_map);
    if (s_win_right) delwin(s_win_right);
    if (s_win_input) delwin(s_win_input);
    endwin();
    s_active = 0;
}

// ── Info buffer ───────────────────────────────────────────────────────────────

void render_info_clear(void)
{
    s_info_count = 0;
}

void render_info_push(const char *fmt, ...)
{
    if (s_info_count >= INFO_LINES_MAX)
        return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_info_buf[s_info_count++], 95, fmt, args);
    va_end(args);
}

// ── Panel: Help (left) ────────────────────────────────────────────────────────

static void draw_help(void)
{
    if (!s_win_help)
        return;
    werase(s_win_help);
    box(s_win_help, 0, 0);
    wattron(s_win_help, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(s_win_help, 0, 2, "[ Commandes ]");
    wattroff(s_win_help, COLOR_PAIR(CP_TITLE) | A_BOLD);

    int row = 1;
    int w = WIN_HELP_W - 2;
    int max_row = getmaxy(s_win_help) - 1;

#define HELP_H(label) \
    if (row < max_row) { \
        wattron(s_win_help, A_BOLD); \
        mvwprintw(s_win_help, row++, 1, "%-*.*s", w, w, label); \
        wattroff(s_win_help, A_BOLD); \
    }
#define HELP_L(line) \
    if (row < max_row) \
        mvwprintw(s_win_help, row++, 1, "%-*.*s", w, w, line);

    HELP_H("-- DEPLACEMENT --")
    HELP_L(" move <id> <x> <y>")
    HELP_L("   deplacer une unite")
    HELP_L(" attack <id> <ennemi>")
    HELP_L("   attaquer un ennemi")
    HELP_H("-- VILLES --")
    HELP_L(" found <nom>")
    HELP_L("   fonder (Settler requis)")
    HELP_L(" build <cid> unit|bld <n>")
    HELP_L("   definir la production")
    HELP_H("-- TECH / RELIGION --")
    HELP_L(" research <tech>")
    HELP_L("   lancer une recherche")
    HELP_L(" found_religion <nom>")
    HELP_H("-- INFORMATIONS --")
    HELP_L(" info unit <id>")
    HELP_L(" info city <id>")
    HELP_L(" tech   (arbre technos)")
    HELP_L(" help   (aide complete)")
    HELP_H("-- JEU --")
    HELP_L(" next       fin du tour")
    HELP_L(" clear")
    HELP_L("   vide les evenements")
    HELP_L(" save/load <fichier>")
    HELP_L(" quit")

    if (row < max_row) mvwhline(s_win_help, row++, 1, ACS_HLINE, w);
    HELP_L(" P=vous    E=ennemi")
    HELP_L(" @=ville   #=v.IA")
    HELP_L(" . plaine  ^ montagne")
    HELP_L(" ~ eau")

#undef HELP_H
#undef HELP_L

    wrefresh(s_win_help);
}

// ── Panel: Map (center) ───────────────────────────────────────────────────────

static void tile_char(GameState *gs, int x, int y, chtype *ch, int *pair)
{
    Tile *t = &gs->map.grid[y][x];
    *pair = 0;

    if (t->unit_id != NO_ID) {
        Unit *u = unit_get(gs, t->unit_id);
        if (u) {
            *ch = (u->owner == PLAYER_OWNER_ID) ? 'P' : 'E';
            *pair = (u->owner == PLAYER_OWNER_ID) ? CP_PLAYER : CP_ENEMY;
            return;
        }
    }
    if (t->city_id != NO_ID) {
        City *c = city_get(gs, t->city_id);
        if (c) {
            *ch = (c->owner == PLAYER_OWNER_ID) ? '@' : '#';
            *pair = (c->owner == PLAYER_OWNER_ID) ? CP_PLAYER : CP_ENEMY;
            return;
        }
    }
    switch (t->type) {
    case TERRAIN_PLAIN:    *ch = '.'; *pair = CP_PLAIN; break;
    case TERRAIN_MOUNTAIN: *ch = '^'; *pair = CP_MOUNT; break;
    case TERRAIN_WATER:    *ch = '~'; *pair = CP_WATER; break;
    default:               *ch = '?'; break;
    }
}

static void draw_map(GameState *gs)
{
    if (!s_win_map)
        return;
    werase(s_win_map);
    box(s_win_map, 0, 0);
    wattron(s_win_map, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(s_win_map, 0, 2, "[ Carte %dx%d ]",
        gs->map.width, gs->map.height);
    wattroff(s_win_map, COLOR_PAIR(CP_TITLE) | A_BOLD);

    int vw = gs->map.width;
    int vh = gs->map.height;
    int max_vw = (getmaxx(s_win_map) - 4) / 2;
    int max_vh = getmaxy(s_win_map) - 4;
    if (vw > max_vw) vw = max_vw;
    if (vh > max_vh) vh = max_vh;

    // Column header — 2 chars per column
    mvwprintw(s_win_map, 1, 1, "   ");
    for (int vx = 0; vx < vw; vx++) {
        if (vx % 10 == 0)
            wprintw(s_win_map, "%02d", vx);
        else if (vx % 5 == 0)
            wprintw(s_win_map, "::");
        else
            wprintw(s_win_map, "  ");
    }

    // Tile rows — 2 chars per tile for a square appearance
    for (int vy = 0; vy < vh; vy++) {
        if (vy % 5 == 0)
            mvwprintw(s_win_map, vy + 2, 1, "%2d ", vy);
        else
            mvwprintw(s_win_map, vy + 2, 1, "   ");
        for (int vx = 0; vx < vw; vx++) {
            chtype ch;
            int pair;
            tile_char(gs, vx, vy, &ch, &pair);
            if (pair)
                wattron(s_win_map, COLOR_PAIR(pair));
            waddch(s_win_map, ch);
            waddch(s_win_map, ' ');
            if (pair)
                wattroff(s_win_map, COLOR_PAIR(pair));
        }
    }

    wrefresh(s_win_map);
}

// ── Panel: Right (status / cities / units / events) ───────────────────────────

static int draw_right_status(GameState *gs, int row)
{
    int w = getmaxx(s_win_right) - 2;

    wattron(s_win_right, A_BOLD);
    mvwprintw(s_win_right, row++, 1, "=== Statut ===");
    wattroff(s_win_right, A_BOLD);

    mvwprintw(s_win_right, row++, 1, "Tour:  %d / %d",
        gs->current_turn, gs->config.max_turns);
    mvwprintw(s_win_right, row++, 1, "Or:    %-5d (+%d/tour)",
        gs->player.gold, gs->player.gold_per_turn);
    mvwprintw(s_win_right, row++, 1, "Sci:   %-5d (+%d/tour)",
        gs->player.science, gs->player.science_per_turn);
    mvwprintw(s_win_right, row++, 1, "Cul:   %-5d Score: %d",
        gs->player.culture_points, gs->player.score);

    if (gs->player.research.current_tech_id != NO_ID) {
        const TechDef *def = tech_tree_get(gs->player.research.current_tech_id);
        if (def) {
            int prog = 0;
            for (int i = 0; i < gs->player.techs.count; i++) {
                if (gs->player.techs.data[i].tech_id == gs->player.research.current_tech_id)
                    prog = gs->player.techs.data[i].progress;
            }
            mvwprintw(s_win_right, row++, 1, "Rech:  %-*.*s (%d/%d)",
                w - 18, w - 18, def->name, prog, def->base_cost);
        }
    } else {
        mvwprintw(s_win_right, row++, 1, "Rech:  aucune");
    }

    if (gs->player.rocket.unlocked) {
        mvwprintw(s_win_right, row++, 1, "Fusee: etape %d/%d  +%d",
            gs->player.rocket.stages_completed, ROCKET_TOTAL_STAGES,
            gs->player.rocket.progress);
    }
    mvwhline(s_win_right, row++, 1, ACS_HLINE, w);
    return row;
}

static int draw_right_cities(GameState *gs, int row)
{
    int w = getmaxx(s_win_right) - 2;
    int max_row = getmaxy(s_win_right) - 12; // leave room for units + events

    wattron(s_win_right, A_BOLD);
    mvwprintw(s_win_right, row++, 1, "=== Villes ===");
    wattroff(s_win_right, A_BOLD);

    for (int i = 0; i < gs->cities.count && row < max_row; i++) {
        City *c = &gs->cities.data[i];
        if (!c->is_active || c->owner != PLAYER_OWNER_ID)
            continue;
        mvwprintw(s_win_right, row++, 1, "#%-2d %-16.16s (%2d,%2d) pop:%d",
            c->id, c->name, c->x, c->y, c->population);
        if (c->prod_project != NO_ID && c->prod_type != PROD_NONE && row < max_row) {
            int cost = 0;
            const char *proj_name = "?";
            if (c->prod_type == PROD_UNIT) {
                const UnitTemplate *t = unit_template_get(c->prod_project);
                if (t) { cost = t->prod_cost; proj_name = t->name; }
            } else {
                const BuildingTemplate *t = building_template_get(c->prod_project);
                if (t) { cost = t->prod_cost; proj_name = t->name; }
            }
            if (cost > 0) {
                int filled = c->production * 8 / cost;
                char bar[9];
                for (int b = 0; b < 8; b++)
                    bar[b] = b < filled ? '#' : '.';
                bar[8] = '\0';
                mvwprintw(s_win_right, row++, 3, "->%-12.12s [%s] %d/%d",
                    proj_name, bar, c->production, cost);
            }
        }
    }
    mvwhline(s_win_right, row++, 1, ACS_HLINE, w);
    return row;
}

static int draw_right_units(GameState *gs, int row)
{
    int w = getmaxx(s_win_right) - 2;
    int max_row = getmaxy(s_win_right) - 12; // leave room for enemies + events

    wattron(s_win_right, A_BOLD);
    mvwprintw(s_win_right, row++, 1, "=== Unites ===");
    wattroff(s_win_right, A_BOLD);

    for (int i = 0; i < gs->units.count && row < max_row; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner != PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        const char *name = tmpl ? tmpl->name : "?";
        mvwprintw(s_win_right, row++, 1, "#%-2d %-14.14s (%2d,%2d) hp:%-3d mv:%d",
            u->id, name, u->x, u->y, u->hp, u->moves_left);
    }
    mvwhline(s_win_right, row++, 1, ACS_HLINE, w);
    return row;
}

static int draw_right_enemies(GameState *gs, int row)
{
    int w = getmaxx(s_win_right) - 2;
    int max_row = getmaxy(s_win_right) - 6;

    wattron(s_win_right, COLOR_PAIR(CP_ENEMY) | A_BOLD);
    mvwprintw(s_win_right, row++, 1, "=== Ennemis ===");
    wattroff(s_win_right, COLOR_PAIR(CP_ENEMY) | A_BOLD);

    bool any = false;
    for (int i = 0; i < gs->units.count && row < max_row; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner == PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        const char *uname = tmpl ? tmpl->name : "?";
        const char *faction = "?";
        for (int j = 0; j < gs->ai_factions.count; j++) {
            if (gs->ai_factions.data[j].id == u->owner) {
                faction = gs->ai_factions.data[j].name;
                break;
            }
        }
        wattron(s_win_right, COLOR_PAIR(CP_ENEMY));
        mvwprintw(s_win_right, row++, 1, "#%-2d %-14.14s (%2d,%2d) hp:%-3d [%s]",
            u->id, uname, u->x, u->y, u->hp, faction);
        wattroff(s_win_right, COLOR_PAIR(CP_ENEMY));
        any = true;
    }
    if (!any && row < max_row)
        mvwprintw(s_win_right, row++, 1, "aucun ennemi visible");
    mvwhline(s_win_right, row++, 1, ACS_HLINE, w);
    return row;
}

static void draw_right_events(GameState *gs, int row)
{
    int max_row = getmaxy(s_win_right) - 1;
    int w = getmaxx(s_win_right) - 2;

    wattron(s_win_right, A_BOLD);
    mvwprintw(s_win_right, row++, 1, "=== Evenements ===");
    wattroff(s_win_right, A_BOLD);

    if (s_info_count > 0) {
        for (int i = 0; i < s_info_count && row < max_row; i++)
            mvwprintw(s_win_right, row++, 1, "%-*.*s", w, w, s_info_buf[i]);
        return;
    }
    int avail = max_row - row;
    int start = 0;
    if (gs->events.count > avail)
        start = gs->events.count - avail;
    for (int i = start; i < gs->events.count && row < max_row; i++) {
        GameEvent *ev = &gs->events.data[i];
        int pair = (ev->owner == PLAYER_OWNER_ID) ? CP_PLAYER : CP_ENEMY;
        wattron(s_win_right, COLOR_PAIR(pair));
        mvwprintw(s_win_right, row++, 1, "%-*.*s", w, w, ev->msg);
        wattroff(s_win_right, COLOR_PAIR(pair));
    }
}

static void draw_right(GameState *gs)
{
    if (!s_win_right)
        return;
    werase(s_win_right);
    box(s_win_right, 0, 0);
    wattron(s_win_right, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(s_win_right, 0, 2, "[ Infos ]");
    wattroff(s_win_right, COLOR_PAIR(CP_TITLE) | A_BOLD);

    int row = 1;
    row = draw_right_status(gs, row);
    row = draw_right_cities(gs, row);
    row = draw_right_units(gs, row);
    row = draw_right_enemies(gs, row);
    draw_right_events(gs, row);
    wrefresh(s_win_right);
}

// ── Input bar ─────────────────────────────────────────────────────────────────

static void draw_input_bar(void)
{
    if (!s_win_input)
        return;
    werase(s_win_input);
    mvwhline(s_win_input, 0, 0, ACS_HLINE, COLS);
    wrefresh(s_win_input);
}

// ── Public API ────────────────────────────────────────────────────────────────

void render_full(GameState *gs)
{
    s_current_gs = gs;
    if (!s_active) {
        printf("\n=== Tour %d ===\n", gs->current_turn);
        return;
    }
    if (COLS < WIN_HELP_W + WIN_MAP_W + 20 || LINES < 30) {
        mvprintw(0, 0, "Terminal trop petit (min 100x25). Agrandissez la fenetre.");
        refresh();
        return;
    }
    static int last_cols = 0;
    static int last_lines = 0;
    if (COLS != last_cols || LINES != last_lines) {
        create_windows();
        last_cols = COLS;
        last_lines = LINES;
    }
    draw_help();
    draw_map(gs);
    draw_right(gs);
    draw_input_bar();
}

void render_read_input(char *buf, int len)
{
    if (!s_active || !s_win_input) {
        printf("> ");
        fflush(stdout);
        if (fgets(buf, len, stdin)) {
            int l = (int)strlen(buf);
            while (l > 0 && (buf[l - 1] == '\n' || buf[l - 1] == '\r'))
                buf[--l] = '\0';
        }
        return;
    }
    werase(s_win_input);
    mvwhline(s_win_input, 0, 0, ACS_HLINE, COLS);
    mvwprintw(s_win_input, 1, 0, "> ");
    wrefresh(s_win_input);
    echo();
    curs_set(1);
    int result = wgetnstr(s_win_input, buf, len - 1);
    noecho();
    curs_set(0);
    // EOF on stdin (e.g. piped input exhausted): quit gracefully
    if (result == ERR)
        strncpy(buf, "quit", (size_t)len - 1);
}

void render_message(GameState *gs, const char *fmt, ...)
{
    if (!gs)
        return;
    GameEvent ev;
    ev.type = EVENT_UNIT_KILLED; // generic type for display-only messages
    ev.owner = PLAYER_OWNER_ID;
    va_list args;
    va_start(args, fmt);
    vsnprintf(ev.msg, sizeof(ev.msg), fmt, args);
    va_end(args);
    GameEventArray_push(&gs->events, ev);
}

// ── Menu helper ───────────────────────────────────────────────────────────────

static int menu_select(const char *title, const char **items, int count, int default_sel)
{
    int sel = default_sel;
    int start_y = LINES / 2 - count / 2 - 3;
    int start_x = COLS / 4;

    if (start_y < 1)
        start_y = 1;
    nodelay(stdscr, FALSE);
    keypad(stdscr, TRUE);
    while (1) {
        werase(stdscr);
        wattron(stdscr, COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvprintw(start_y, start_x, "=== %s ===", title);
        wattroff(stdscr, COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvprintw(start_y + 1, start_x, "Haut/Bas pour naviguer, Entree pour confirmer");
        for (int i = 0; i < count; i++) {
            if (i == sel)
                attron(A_REVERSE);
            mvprintw(start_y + 3 + i, start_x, "  %s", items[i]);
            if (i == sel)
                attroff(A_REVERSE);
        }
        refresh();
        int ch = getch();
        if (ch == KEY_UP && sel > 0)
            sel--;
        else if (ch == KEY_DOWN && sel < count - 1)
            sel++;
        else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER)
            break;
    }
    return sel;
}

// ── Start menu ────────────────────────────────────────────────────────────────

int render_start_menu(GameConfig *config, int *civ_id_out)
{
    if (!s_active) {
        *civ_id_out = 0;
        return 0;
    }
    // Toutes les conditions de victoire sont verifiees a chaque tour
    config->victory_type = VICTORY_SCORE; // valeur par defaut, non utilisee pour filtrer

    // Phase 2: Difficulty
    const char *diff[] = {
        "Peaceful  - 0 IA  (exploration libre)",
        "Easy      - 1 IA  (aggression faible)",
        "Medium    - 3 IA  (aggression moderee)",
        "Hard      - 5 IA  (+ tech de depart)",
        "Extreme   - 7 IA  (+ tech + aggression max)"
    };
    int diff_ai[] = {0, 1, 3, 5, 7};
    int sel = 2;
    sel = menu_select("Difficulte", diff, 5, sel);
    config->difficulty = sel;
    config->num_ai_factions = diff_ai[sel];

    // Phase 3: Civilization
    // Civilization names/descriptions are hardcoded here to avoid including civ.h in render.c
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
    sel = 0;
    sel = menu_select("Choisissez votre civilisation", civs, 10, sel);
    *civ_id_out = sel;
    config->civ_id = sel;
    return 0;
}

// ── End screen ────────────────────────────────────────────────────────────────

bool render_end_screen(GameState *gs)
{
    if (!s_active)
        return false;
    nodelay(stdscr, FALSE);
    keypad(stdscr, TRUE);
    while (1) {
        werase(stdscr);
        int row = 2;
        int col = COLS / 6;
        wattron(stdscr, COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvprintw(row++, col, "=== PARTIE TERMINEE  - Tour %d/%d ===",
            gs->current_turn, gs->config.max_turns);
        wattroff(stdscr, COLOR_PAIR(CP_TITLE) | A_BOLD);
        row++;
        if (gs->victory.achieved) {
            if (gs->victory.winner_owner == PLAYER_OWNER_ID) {
                wattron(stdscr, COLOR_PAIR(CP_PLAYER) | A_BOLD);
                mvprintw(row++, col, "*** VICTOIRE ! Vous avez gagne ! ***");
                wattroff(stdscr, COLOR_PAIR(CP_PLAYER) | A_BOLD);
            } else {
                wattron(stdscr, COLOR_PAIR(CP_ENEMY) | A_BOLD);
                mvprintw(row++, col, "*** DEFAITE  - Joueur #%d a gagne ***",
                    gs->victory.winner_owner);
                wattroff(stdscr, COLOR_PAIR(CP_ENEMY) | A_BOLD);
            }
        } else {
            mvprintw(row++, col, "Partie terminee (abandon).");
        }
        row++;
        // Scores
        mvprintw(row++, col, "--- Scores finaux ---");
        // Player
        int p_cities = 0;
        int p_units = 0;
        for (int i = 0; i < gs->cities.count; i++) {
            if (gs->cities.data[i].is_active && gs->cities.data[i].owner == PLAYER_OWNER_ID)
                p_cities++;
        }
        for (int i = 0; i < gs->units.count; i++) {
            if (gs->units.data[i].is_active && gs->units.data[i].owner == PLAYER_OWNER_ID)
                p_units++;
        }
        mvprintw(row++, col,
            "  Vous        : %4d pts   %d villes  %d unites  culture:%d",
            gs->player.score, p_cities, p_units, gs->player.culture_points);
        // AI factions
        for (int i = 0; i < gs->ai_factions.count; i++) {
            AIFaction *f = &gs->ai_factions.data[i];
            int fc = 0;
            int fu = 0;
            for (int j = 0; j < gs->cities.count; j++) {
                if (gs->cities.data[j].is_active && gs->cities.data[j].owner == f->id)
                    fc++;
            }
            for (int j = 0; j < gs->units.count; j++) {
                if (gs->units.data[j].is_active && gs->units.data[j].owner == f->id)
                    fu++;
            }
            if (f->is_eliminated) {
                mvprintw(row++, col, "  %-12s: ELIMINE", f->name);
            } else {
                mvprintw(row++, col,
                    "  %-12s: %4d pts   %d villes  %d unites",
                    f->name, f->score, fc, fu);
            }
        }
        row++;
        mvprintw(row++, col, "[R] Rejouer     [Q] Quitter");
        refresh();
        int ch = getch();
        if (ch == 'r' || ch == 'R')
            return true;
        if (ch == 'q' || ch == 'Q' || ch == 27)
            return false;
    }
}
