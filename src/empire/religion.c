#include <string.h>
#include "empire/religion.h"
#include "empire/empire.h"
#include "world/map.h"
#include "world/tile.h"
#include "entities/unit.h"

int religion_of_owner(GameState *gs, int owner)
{
    for (int i = 0; i < gs->religions.count; i++) {
        if (gs->religions.data[i].founder_owner == owner)
            return gs->religions.data[i].id;
    }
    return NO_ID;
}

int religion_found(GameState *gs, int owner, const char *name)
{
    bool *abilities = owner_abilities(gs, owner);
    if (!abilities || !abilities[ABILITY_FOUND_RELIGION])
        return NO_ID;
    // Can only found one religion per owner
    if (religion_of_owner(gs, owner) != NO_ID)
        return NO_ID;
    Religion r;
    r.id = gs->religions.count;
    strncpy(r.name, name, 31);
    r.name[31] = '\0';
    r.founder_owner = owner;
    r.converted_tiles = 0;
    ReligionArray_push(&gs->religions, r);
    // Consume the ability (one-shot)
    abilities[ABILITY_FOUND_RELIGION] = false;
    return r.id;
}

int religion_count_tiles(GameState *gs, int religion_id)
{
    int count = 0;

    for (int y = 0; y < gs->map.height; y++) {
        for (int x = 0; x < gs->map.width; x++) {
            if (gs->map.grid[y][x].religion_id == religion_id)
                count++;
        }
    }
    return count;
}

static void spread_religion_from(GameState *gs, int cx, int cy, int religion_id)
{
    int dx[] = {0, 0, 1, -1, 1, 1, -1, -1};
    int dy[] = {1, -1, 0, 0, 1, -1, 1, -1};

    for (int d = 0; d < 8; d++) {
        Tile *t = map_get(gs, cx + dx[d], cy + dy[d]);
        if (t && t->type != TERRAIN_WATER)
            tile_set_religion(gs, cx + dx[d], cy + dy[d], religion_id);
    }
}

void religion_spread_tick(GameState *gs)
{
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        if (!tmpl || tmpl->role != ROLE_MISSIONARY)
            continue;
        int rid = religion_of_owner(gs, u->owner);
        if (rid == NO_ID)
            continue;
        spread_religion_from(gs, u->x, u->y, rid);
    }
    // Update cached converted_tiles count for each religion
    for (int i = 0; i < gs->religions.count; i++) {
        gs->religions.data[i].converted_tiles =
            religion_count_tiles(gs, gs->religions.data[i].id);
    }
}

bool religion_has_victory(GameState *gs)
{
    int non_water = map_count_non_water(gs);
    if (non_water == 0)
        return false;
    for (int i = 0; i < gs->religions.count; i++) {
        int tiles = gs->religions.data[i].converted_tiles;
        if (tiles * 100 / non_water >= RELIGION_VICTORY_THRESHOLD)
            return true;
    }
    return false;
}
