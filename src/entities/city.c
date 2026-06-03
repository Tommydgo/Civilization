#include <string.h>
#include <stdio.h>
#include "entities/city.h"
#include "entities/unit.h"
#include "world/map.h"
#include "tech/tech.h"
#include "events/event.h"

const BuildingTemplate BUILDING_TEMPLATES[] = {
    {0, "Grenier", 30, 2, 0, 0, 0, 0}, // food+2, tech: Agriculture
    {1, "Usine",   80, 0, 3, 1, 0, 6}, // prod+3 sci+1, tech: Industrie
};

const int BUILDING_TEMPLATE_COUNT = 2;

static int s_next_city_id = 0;

const BuildingTemplate *building_template_get(int id)
{
    if (id < 0 || id >= BUILDING_TEMPLATE_COUNT)
        return NULL;
    return &BUILDING_TEMPLATES[id];
}

City *city_get(GameState *gs, int city_id)
{
    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].id == city_id && gs->cities.data[i].is_active)
            return &gs->cities.data[i];
    }
    return NULL;
}

int city_found(GameState *gs, int x, int y, int owner, const char *name)
{
    Tile *t = map_get(gs, x, y);
    if (!t || t->type == TERRAIN_WATER || t->city_id != NO_ID)
        return NO_ID;
    City c;
    memset(&c, 0, sizeof(c));
    c.id = s_next_city_id++;
    strncpy(c.name, name, 31);
    c.name[31] = '\0';
    c.x = x;
    c.y = y;
    c.owner = owner;
    c.population = 1;
    c.food = 0;
    c.food_cap = 10;
    c.production = 0;
    c.prod_project = NO_ID;
    c.prod_type = PROD_NONE;
    c.religion_id = NO_ID;
    c.culture_points = 0;
    c.is_active = true;
    CityBuildingArray_init(&c.buildings);
    CityArray_push(&gs->cities, c);
    t->city_id = c.id;
    t->culture_owner = owner;
    event_push(gs, EVENT_CITY_FOUNDED, owner, "Ville '%s' fondee en (%d,%d)", name, x, y);
    return c.id;
}

static int city_food_yield(GameState *gs, City *c)
{
    Tile *t = map_get(gs, c->x, c->y);
    int yield = 0;

    if (t)
        yield += TERRAIN_STATS[t->type].food_mod;
    for (int i = 0; i < c->buildings.count; i++) {
        if (!c->buildings.data[i].is_active)
            continue;
        const BuildingTemplate *b = building_template_get(c->buildings.data[i].building_id);
        if (b)
            yield += b->food_bonus;
    }
    if (c->owner == PLAYER_OWNER_ID)
        yield += gs->player.city_food_bonus;
    return yield;
}

static int city_prod_yield(GameState *gs, City *c)
{
    Tile *t = map_get(gs, c->x, c->y);
    int yield = 1;

    if (t)
        yield += TERRAIN_STATS[t->type].prod_mod;
    for (int i = 0; i < c->buildings.count; i++) {
        if (!c->buildings.data[i].is_active)
            continue;
        const BuildingTemplate *b = building_template_get(c->buildings.data[i].building_id);
        if (b)
            yield += b->prod_bonus;
    }
    if (c->owner == PLAYER_OWNER_ID)
        yield += gs->player.city_prod_bonus;
    return yield;
}

static void city_complete_unit(GameState *gs, City *c)
{
    // Spawn on city tile; if occupied find an adjacent free land tile
    int sx = c->x;
    int sy = c->y;
    Tile *t = map_get(gs, sx, sy);

    if (t && t->unit_id != NO_ID) {
        int dx[] = {0, 0, 1, -1};
        int dy[] = {1, -1, 0, 0};
        int found = 0;
        for (int i = 0; i < 4 && !found; i++) {
            Tile *adj = map_get(gs, sx + dx[i], sy + dy[i]);
            if (adj && adj->unit_id == NO_ID && adj->type != TERRAIN_WATER) {
                sx = sx + dx[i];
                sy = sy + dy[i];
                found = 1;
            }
        }
        if (!found)
            return; // No spawn space available
    }
    int uid = unit_create(gs, c->prod_project, sx, sy, c->owner);
    if (uid != NO_ID) {
        const UnitTemplate *tmpl = unit_template_get(c->prod_project);
        event_push(gs, EVENT_UNIT_CREATED, c->owner,
            "%s produit dans %s", tmpl ? tmpl->name : "unite", c->name);
    }
}

static void city_complete_building(City *c)
{
    // Check not already built
    for (int i = 0; i < c->buildings.count; i++) {
        if (c->buildings.data[i].building_id == c->prod_project)
            return;
    }
    CityBuilding b;
    b.building_id = c->prod_project;
    b.is_active = true;
    CityBuildingArray_push(&c->buildings, b);
}

void city_tick(GameState *gs, int city_id)
{
    City *c = city_get(gs, city_id);
    if (!c)
        return;
    // Food
    int net_food = city_food_yield(gs, c) - c->population;
    c->food += net_food;
    if (c->food >= c->food_cap) {
        c->population++;
        c->food = 0;
        c->food_cap = 10 * c->population;
    }
    if (c->food < 0)
        c->food = 0;
    // Culture points accumulate from buildings
    for (int i = 0; i < c->buildings.count; i++) {
        if (!c->buildings.data[i].is_active)
            continue;
        const BuildingTemplate *b = building_template_get(c->buildings.data[i].building_id);
        if (b)
            c->culture_points += b->culture_bonus;
    }
    // Production
    if (c->prod_project == NO_ID || c->prod_type == PROD_NONE)
        return;
    c->production += city_prod_yield(gs, c);
    int cost = 0;
    if (c->prod_type == PROD_UNIT) {
        const UnitTemplate *tmpl = unit_template_get(c->prod_project);
        if (tmpl)
            cost = tmpl->prod_cost;
    } else if (c->prod_type == PROD_BUILDING) {
        const BuildingTemplate *tmpl = building_template_get(c->prod_project);
        if (tmpl)
            cost = tmpl->prod_cost;
    }
    if (cost > 0 && c->production >= cost) {
        if (c->prod_type == PROD_UNIT)
            city_complete_unit(gs, c);
        else if (c->prod_type == PROD_BUILDING)
            city_complete_building(c);
        c->production = 0;
        c->prod_project = NO_ID;
        c->prod_type = PROD_NONE;
    }
}

bool city_can_produce(GameState *gs, int city_id, int project_id, ProdProjectType type)
{
    City *c = city_get(gs, city_id);
    if (!c)
        return false;
    if (type == PROD_UNIT) {
        const UnitTemplate *tmpl = unit_template_get(project_id);
        if (!tmpl)
            return false;
        if (tmpl->required_tech_id != NO_ID)
            return tech_is_researched(gs, c->owner, tmpl->required_tech_id);
        return true;
    }
    if (type == PROD_BUILDING) {
        const BuildingTemplate *tmpl = building_template_get(project_id);
        if (!tmpl)
            return false;
        // Check not already built
        for (int i = 0; i < c->buildings.count; i++) {
            if (c->buildings.data[i].building_id == project_id)
                return false;
        }
        if (tmpl->required_tech_id != NO_ID)
            return tech_is_researched(gs, c->owner, tmpl->required_tech_id);
        return true;
    }
    return false;
}

bool city_set_project(GameState *gs, int city_id, int project_id, ProdProjectType type)
{
    if (!city_can_produce(gs, city_id, project_id, type))
        return false;
    City *c = city_get(gs, city_id);
    if (!c)
        return false;
    c->prod_project = project_id;
    c->prod_type = type;
    c->production = 0;
    return true;
}

void city_reset_id_counter(GameState *gs)
{
    int max_id = 0;

    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].id > max_id)
            max_id = gs->cities.data[i].id;
    }
    s_next_city_id = max_id + 1;
}
