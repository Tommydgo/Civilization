#include <string.h>
#include "entities/unit.h"
#include "world/tile.h"
#include "world/map.h"
#include "events/event.h"

const UnitTemplate UNIT_TEMPLATES[] = {
    {0, "Guerrier",        20, 10,  4, 3, 2, ROLE_WARRIOR,         2},
    {1, "Cavalier",        40, 12,  6, 4, 3, ROLE_WARRIOR,         4},
    {2, "Settler",         30,  5,  0, 1, 2, ROLE_SETTLER,         NO_ID},
    {3, "Missionnaire",    25,  5,  0, 1, 2, ROLE_MISSIONARY,      1},
    {4, "Ing. Fusee",      50,  5,  0, 1, 1, ROLE_ROCKET_ENGINEER, 7},
};

const int UNIT_TEMPLATE_COUNT = 5;

static int s_next_unit_id = 0;

const UnitTemplate *unit_template_get(int id)
{
    if (id < 0 || id >= UNIT_TEMPLATE_COUNT)
        return NULL;
    return &UNIT_TEMPLATES[id];
}

Unit *unit_get(GameState *gs, int unit_id)
{
    for (int i = 0; i < gs->units.count; i++) {
        if (gs->units.data[i].id == unit_id && gs->units.data[i].is_active)
            return &gs->units.data[i];
    }
    return NULL;
}

int unit_create(GameState *gs, int template_id, int x, int y, int owner)
{
    const UnitTemplate *tmpl = unit_template_get(template_id);
    if (!tmpl)
        return NO_ID;
    Tile *t = map_get(gs, x, y);
    if (!t || t->unit_id != NO_ID)
        return NO_ID;
    Unit u;
    memset(&u, 0, sizeof(u));
    u.id = s_next_unit_id++;
    u.template_id = template_id;
    u.x = x;
    u.y = y;
    u.owner = owner;
    u.hp = tmpl->max_hp;
    u.moves_left = tmpl->movement;
    u.is_active = true;
    UnitArray_push(&gs->units, u);
    t->unit_id = u.id;
    return u.id;
}

bool unit_move(GameState *gs, int unit_id, int tx, int ty)
{
    Unit *u = unit_get(gs, unit_id);
    if (!u || u->moves_left <= 0)
        return false;
    int dx = tx - u->x;
    int dy = ty - u->y;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx + dy != 1)
        return false;
    if (!tile_is_passable(gs, tx, ty, unit_id))
        return false;
    Tile *dest = map_get(gs, tx, ty);
    if (!dest || dest->unit_id != NO_ID)
        return false;
    Tile *src = map_get(gs, u->x, u->y);
    if (src)
        src->unit_id = NO_ID;
    dest->unit_id = unit_id;
    // If moving onto a city owned by another player, capture it
    if (dest->city_id != NO_ID) {
        for (int i = 0; i < gs->cities.count; i++) {
            City *c = &gs->cities.data[i];
            if (c->id != dest->city_id || c->owner == u->owner || !c->is_active)
                continue;
            int prev_owner = c->owner;
            c->owner = u->owner;
            if (u->owner == PLAYER_OWNER_ID)
                event_push(gs, EVENT_CITY_CAPTURED, u->owner,
                    "Vous avez capture la ville %s !", c->name);
            else
                event_push(gs, EVENT_CITY_CAPTURED, prev_owner,
                    "IA #%d a capture votre ville %s !", u->owner, c->name);
            break;
        }
    }
    u->x = tx;
    u->y = ty;
    u->moves_left--;
    return true;
}

void unit_attack(GameState *gs, int attacker_id, int defender_id)
{
    Unit *att = unit_get(gs, attacker_id);
    Unit *def = unit_get(gs, defender_id);
    if (!att || !def)
        return;
    if (att->owner == def->owner)
        return;
    const UnitTemplate *att_tmpl = unit_template_get(att->template_id);
    const UnitTemplate *def_tmpl = unit_template_get(def->template_id);
    if (!att_tmpl || !def_tmpl)
        return;
    Tile *def_tile = map_get(gs, def->x, def->y);
    int terrain_bonus = 0;
    if (def_tile)
        terrain_bonus = TERRAIN_STATS[def_tile->type].defense_bonus;
    int att_bonus = (att->owner == PLAYER_OWNER_ID) ? gs->player.unit_attack_bonus : 0;
    int def_bonus = (def->owner == PLAYER_OWNER_ID) ? gs->player.unit_defense_bonus : 0;
    int eff_attack = att_tmpl->attack + att_bonus;
    int eff_defense = def_tmpl->defense + def_bonus + terrain_bonus;
    int damage_to_def = eff_attack - eff_defense / 2;
    if (damage_to_def < 1)
        damage_to_def = 1;
    int damage_to_att = eff_defense / 4;
    if (damage_to_att < 1)
        damage_to_att = 1;
    def->hp -= damage_to_def;
    att->hp -= damage_to_att;
    att->moves_left = 0;
    if (def->hp <= 0) {
        const UnitTemplate *dt = unit_template_get(def->template_id);
        if (def->owner == PLAYER_OWNER_ID)
            event_push(gs, EVENT_UNIT_KILLED, att->owner,
                "Votre %s a ete tue par IA #%d !", dt ? dt->name : "unite", att->owner);
        else
            event_push(gs, EVENT_UNIT_KILLED, att->owner,
                "Vous avez tue un %s (IA #%d)", dt ? dt->name : "ennemi", def->owner);
        unit_kill(gs, defender_id);
    }
    if (att->hp <= 0) {
        const UnitTemplate *at = unit_template_get(att->template_id);
        event_push(gs, EVENT_UNIT_KILLED, def->owner,
            "%s #%d elimine pendant le combat.", at ? at->name : "unite", att->id);
        unit_kill(gs, attacker_id);
    }
}

void unit_kill(GameState *gs, int unit_id)
{
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (u->id != unit_id)
            continue;
        Tile *t = map_get(gs, u->x, u->y);
        if (t && t->unit_id == unit_id)
            t->unit_id = NO_ID;
        u->is_active = false;
        UnitArray_remove(&gs->units, i);
        return;
    }
}

void unit_reset_id_counter(GameState *gs)
{
    int max_id = 0;

    for (int i = 0; i < gs->units.count; i++) {
        if (gs->units.data[i].id > max_id)
            max_id = gs->units.data[i].id;
    }
    s_next_unit_id = max_id + 1;
}
