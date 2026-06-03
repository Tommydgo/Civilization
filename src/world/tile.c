#include "world/tile.h"
#include "world/map.h"
#include "entities/unit.h"

void tile_set_culture(GameState *gs, int x, int y, int owner)
{
    Tile *t = map_get(gs, x, y);
    if (!t)
        return;
    t->culture_owner = owner;
}

void tile_set_religion(GameState *gs, int x, int y, int religion_id)
{
    Tile *t = map_get(gs, x, y);
    if (!t)
        return;
    t->religion_id = religion_id;
}

void tile_set_unit(GameState *gs, int x, int y, int unit_id)
{
    Tile *t = map_get(gs, x, y);
    if (!t)
        return;
    t->unit_id = unit_id;
}

bool tile_is_passable(GameState *gs, int x, int y, int unit_id)
{
    Tile *t = map_get(gs, x, y);
    if (!t)
        return false;
    if (t->type != TERRAIN_WATER)
        return true;
    // Water tiles require ABILITY_COLONIZE and the unit must be a Settler
    Unit *u = unit_get(gs, unit_id);
    if (!u)
        return false;
    const UnitTemplate *tmpl = unit_template_get(u->template_id);
    if (!tmpl || tmpl->role != ROLE_SETTLER)
        return false;
    if (u->owner == PLAYER_OWNER_ID)
        return gs->player.abilities[ABILITY_COLONIZE];
    for (int i = 0; i < gs->ai_factions.count; i++) {
        if (gs->ai_factions.data[i].id == u->owner)
            return gs->ai_factions.data[i].abilities[ABILITY_COLONIZE];
    }
    return false;
}
