#include <stdio.h>
#include <stdarg.h>
#include "ui/render.h"
#include "entities/unit.h"
#include "entities/city.h"
#include "tech/tech_tree.h"

#define COL_RESET  "\033[0m"
#define COL_PLAIN  "\033[32m"  // green
#define COL_MOUNT  "\033[37m"  // white/gray
#define COL_WATER  "\033[34m"  // blue
#define COL_PLAYER "\033[33m"  // yellow
#define COL_ENEMY  "\033[31m"  // red

// Returns the character and color prefix for a tile.
static void tile_display(GameState *gs, int x, int y, char *ch, const char **col)
{
    Tile *t = &gs->map.grid[y][x];

    // Unit takes visual priority over city and terrain
    if (t->unit_id != NO_ID) {
        Unit *u = unit_get(gs, t->unit_id);
        if (u) {
            *ch = (u->owner == PLAYER_OWNER_ID) ? 'P' : 'E';
            *col = (u->owner == PLAYER_OWNER_ID) ? COL_PLAYER : COL_ENEMY;
            return;
        }
    }
    if (t->city_id != NO_ID) {
        for (int i = 0; i < gs->cities.count; i++) {
            City *c = &gs->cities.data[i];
            if (c->id != t->city_id || !c->is_active)
                continue;
            *ch = (c->owner == PLAYER_OWNER_ID) ? '@' : '#';
            *col = (c->owner == PLAYER_OWNER_ID) ? COL_PLAYER : COL_ENEMY;
            return;
        }
    }
    switch (t->type) {
    case TERRAIN_PLAIN:
        *ch = '.';
        *col = COL_PLAIN;
        break;
    case TERRAIN_MOUNTAIN:
        *ch = '^';
        *col = COL_MOUNT;
        break;
    case TERRAIN_WATER:
        *ch = '~';
        *col = COL_WATER;
        break;
    }
}

void render_map(GameState *gs)
{
    // Column index header
    printf("   ");
    for (int x = 0; x < gs->map.width; x++)
        printf("%2d", x);
    printf("\n");

    for (int y = 0; y < gs->map.height; y++) {
        printf("%2d ", y);
        for (int x = 0; x < gs->map.width; x++) {
            char ch = '?';
            const char *col = COL_RESET;
            tile_display(gs, x, y, &ch, &col);
            printf("%s%c" COL_RESET " ", col, ch);
        }
        printf("\n");
    }
}

void render_status(GameState *gs)
{
    printf("\n--- Turn %d / %d ---\n",
        gs->current_turn, gs->config.max_turns);
    printf("Gold:    %4d  (+%d/turn)\n",
        gs->player.gold, gs->player.gold_per_turn);
    printf("Science: %4d  (+%d/turn)\n",
        gs->player.science, gs->player.science_per_turn);
    printf("Culture: %4d\n", gs->player.culture_points);
    printf("Score:   %4d\n", gs->player.score);
    // Rocket status
    if (gs->player.rocket.unlocked) {
        printf("Rocket:  stage %d/%d  progress %d/%d\n",
            gs->player.rocket.stages_completed,
            ROCKET_TOTAL_STAGES,
            gs->player.rocket.progress,
            gs->player.rocket.stage_cost);
    }
    // Research
    if (gs->player.research.current_tech_id != NO_ID) {
        const TechDef *def = tech_tree_get(gs->player.research.current_tech_id);
        if (def) {
            // Find progress
            int prog = 0;
            for (int i = 0; i < gs->player.techs.count; i++) {
                if (gs->player.techs.data[i].tech_id == gs->player.research.current_tech_id)
                    prog = gs->player.techs.data[i].progress;
            }
            printf("Research: %s (%d/%d)\n", def->name, prog, def->base_cost);
        }
    } else {
        printf("Research: none\n");
    }
    // Cities
    int city_count = 0;
    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].is_active && gs->cities.data[i].owner == PLAYER_OWNER_ID)
            city_count++;
    }
    printf("Cities:  %d\n", city_count);
}

void render_turn_start(GameState *gs)
{
    printf("\n========== TOUR %d ==========\n", gs->current_turn);
}

void render_message(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf(">> ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
