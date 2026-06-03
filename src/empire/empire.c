#include "empire/empire.h"
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "entities/city.h"
#include "entities/unit.h"
#include "world/map.h"
#include "events/event.h"
#include <string.h>

// ── Owner-agnostic accessors ─────────────────────────────────────────────────

static AIFaction *faction_by_owner(GameState *gs, int owner)
{
    for (int i = 0; i < gs->ai_factions.count; i++) {
        if (gs->ai_factions.data[i].id == owner)
            return &gs->ai_factions.data[i];
    }
    return NULL;
}

bool *owner_abilities(GameState *gs, int owner)
{
    if (owner == PLAYER_OWNER_ID)
        return gs->player.abilities;
    AIFaction *f = faction_by_owner(gs, owner);
    return f ? f->abilities : NULL;
}

TechStateArray *owner_techs(GameState *gs, int owner)
{
    if (owner == PLAYER_OWNER_ID)
        return &gs->player.techs;
    AIFaction *f = faction_by_owner(gs, owner);
    return f ? &f->techs : NULL;
}

ResearchQueue *owner_research(GameState *gs, int owner)
{
    if (owner == PLAYER_OWNER_ID)
        return &gs->player.research;
    AIFaction *f = faction_by_owner(gs, owner);
    return f ? &f->research : NULL;
}

RocketProject *owner_rocket(GameState *gs, int owner)
{
    if (owner == PLAYER_OWNER_ID)
        return &gs->player.rocket;
    AIFaction *f = faction_by_owner(gs, owner);
    return f ? &f->rocket : NULL;
}

int *owner_score_ptr(GameState *gs, int owner)
{
    if (owner == PLAYER_OWNER_ID)
        return &gs->player.score;
    AIFaction *f = faction_by_owner(gs, owner);
    return f ? &f->score : NULL;
}

// ── Empire init / free ───────────────────────────────────────────────────────

void empire_init(GameState *gs)
{
    memset(&gs->player, 0, sizeof(Empire));
    gs->player.gold = 10;
    gs->player.science = 0;
    gs->player.gold_per_turn = 0;
    gs->player.science_per_turn = 1;
    gs->player.culture_points = 0;
    gs->player.score = 0;
    gs->player.research.current_tech_id = NO_ID;
    gs->player.research.queue_count = 0;
    gs->player.rocket.unlocked = false;
    gs->player.rocket.stages_completed = 0;
    gs->player.rocket.progress = 0;
    gs->player.rocket.stage_cost = 50;
    tech_init_owner(gs, PLAYER_OWNER_ID);
}

void empire_free(GameState *gs)
{
    TechStateArray_free(&gs->player.techs);
}

void empire_apply_ability(GameState *gs, SpecialAbility ability)
{
    gs->player.abilities[ability] = true;
    if (ability == ABILITY_ROCKET_PROGRAM) {
        gs->player.rocket.unlocked = true;
        gs->player.rocket.stage_cost = 50;
    }
}

// ── Per-turn update ──────────────────────────────────────────────────────────

static void empire_tick_science(GameState *gs)
{
    int sci = 1;

    for (int i = 0; i < gs->cities.count; i++) {
        City *c = &gs->cities.data[i];
        if (!c->is_active || c->owner != PLAYER_OWNER_ID)
            continue;
        sci += c->population;
        for (int j = 0; j < c->buildings.count; j++) {
            if (!c->buildings.data[j].is_active)
                continue;
            const BuildingTemplate *b = building_template_get(c->buildings.data[j].building_id);
            if (b)
                sci += b->science_bonus;
        }
    }
    sci += gs->player.science_per_turn_bonus;
    gs->player.science_per_turn = sci;
    gs->player.science += sci;
}

static void empire_tick_gold(GameState *gs)
{
    int gold = 0;

    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].is_active && gs->cities.data[i].owner == PLAYER_OWNER_ID)
            gold += 2;
    }
    gs->player.gold_per_turn = gold;
    gs->player.gold += gold;
}

static void empire_tick_culture(GameState *gs)
{
    for (int i = 0; i < gs->player.techs.count; i++) {
        if (!gs->player.techs.data[i].researched)
            continue;
        const TechDef *def = tech_tree_get(gs->player.techs.data[i].tech_id);
        if (def && def->culture_bonus > 0)
            gs->player.culture_points += def->culture_bonus;
    }
    gs->player.culture_points += gs->player.culture_per_turn_bonus;
}

static void empire_tick_culture_spread(GameState *gs)
{
    if (!gs->player.abilities[ABILITY_CULTURE_BORDER])
        return;
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    for (int i = 0; i < gs->cities.count; i++) {
        City *c = &gs->cities.data[i];
        if (!c->is_active || c->owner != PLAYER_OWNER_ID)
            continue;
        for (int d = 0; d < 4; d++) {
            Tile *adj = map_get(gs, c->x + dx[d], c->y + dy[d]);
            if (adj && adj->type != TERRAIN_WATER)
                adj->culture_owner = PLAYER_OWNER_ID;
        }
    }
}

static void empire_tick_rocket(GameState *gs)
{
    if (!gs->player.rocket.unlocked)
        return;
    if (gs->player.rocket.stages_completed >= ROCKET_TOTAL_STAGES)
        return;
    int engineers = 0;

    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner != PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        if (tmpl && tmpl->role == ROLE_ROCKET_ENGINEER)
            engineers++;
    }
    gs->player.rocket.progress += engineers;
    if (gs->player.rocket.progress >= gs->player.rocket.stage_cost) {
        gs->player.rocket.stages_completed++;
        gs->player.rocket.progress = 0;
        event_push(gs, EVENT_ROCKET_STAGE, PLAYER_OWNER_ID,
            "Fusee : etape %d/%d completee !",
            gs->player.rocket.stages_completed, ROCKET_TOTAL_STAGES);
    }
}

void empire_tick(GameState *gs)
{
    empire_tick_science(gs);
    empire_tick_gold(gs);
    empire_tick_culture(gs);
    empire_tick_culture_spread(gs);
    empire_tick_rocket(gs);
}
