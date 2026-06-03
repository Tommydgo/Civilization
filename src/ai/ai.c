#include <string.h>
#include "ai/ai.h"
#include "ai/ai_research.h"
#include "entities/unit.h"
#include "entities/city.h"
#include "world/map.h"
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "empire/empire.h"
#include "events/event.h"

// ── Helpers ──────────────────────────────────────────────────────────────────

static int sign(int v)
{
    if (v > 0)
        return 1;
    if (v < 0)
        return -1;
    return 0;
}

// Search outward in a square spiral for a free land tile near (cx, cy).
static bool find_land_near(GameState *gs, int cx, int cy, int *ox, int *oy)
{
    for (int r = 0; r <= 5; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (r > 0 && dx != -r && dx != r && dy != -r && dy != r)
                    continue;
                Tile *t = map_get(gs, cx + dx, cy + dy);
                if (t && t->type != TERRAIN_WATER && t->unit_id == NO_ID && t->city_id == NO_ID) {
                    *ox = cx + dx;
                    *oy = cy + dy;
                    return true;
                }
            }
        }
    }
    return false;
}

// Pick the best unit template the faction can currently produce.
static int best_military_template(GameState *gs, AIFaction *faction)
{
    // Prefer last (strongest) available military template
    int best = NO_ID;

    for (int i = 0; i < UNIT_TEMPLATE_COUNT; i++) {
        const UnitTemplate *tmpl = unit_template_get(i);
        if (!tmpl || tmpl->role != ROLE_WARRIOR)
            continue;
        if (tmpl->required_tech_id != NO_ID
            && !tech_is_researched(gs, faction->id, tmpl->required_tech_id))
            continue;
        best = i;
    }
    return best;
}

// ── Public API ───────────────────────────────────────────────────────────────

void ai_faction_init(GameState *gs, int faction_idx)
{
    AIFaction *f = &gs->ai_factions.data[faction_idx];

    // Owner id = faction array index + 1 (player is 0)
    f->id = faction_idx + 1;
    tech_init_owner(gs, f->id);
    f->research.current_tech_id = NO_ID;
    f->research.queue_count = 0;
    f->next_attack_turn = 5 + faction_idx * 3;
    f->is_eliminated = false;
    f->score = 0;
    memset(f->abilities, 0, sizeof(f->abilities));
    f->rocket.unlocked = false;
    f->rocket.stages_completed = 0;
    f->rocket.progress = 0;
    f->rocket.stage_cost = 50;
    // Spawn initial warrior at faction's designated spawn point
    int sx, sy;
    if (find_land_near(gs, f->spawn_x, f->spawn_y, &sx, &sy))
        unit_create(gs, 0, sx, sy, f->id); // template 0 = Guerrier
}

void ai_spawn_unit(GameState *gs, int faction_idx)
{
    AIFaction *f = &gs->ai_factions.data[faction_idx];
    if (f->is_eliminated)
        return;
    int tmpl_id = best_military_template(gs, f);
    if (tmpl_id == NO_ID)
        tmpl_id = 0; // Fallback to Guerrier
    int sx, sy;
    if (find_land_near(gs, f->spawn_x, f->spawn_y, &sx, &sy)) {
        unit_create(gs, tmpl_id, sx, sy, f->id);
        const UnitTemplate *tmpl = unit_template_get(tmpl_id);
        event_push(gs, EVENT_AI_SPAWNED, f->id,
            "%s spawne un %s !", f->name, tmpl ? tmpl->name : "ennemi");
    }
}

void ai_try_attack(GameState *gs, int faction_idx)
{
    AIFaction *f = &gs->ai_factions.data[faction_idx];
    if (f->is_eliminated)
        return;
    for (int ui = 0; ui < gs->units.count; ui++) {
        Unit *u = &gs->units.data[ui];
        if (!u->is_active || u->owner != f->id || u->moves_left <= 0)
            continue;
        // Find nearest player-owned target (unit or city)
        int target_x = NO_ID;
        int target_y = NO_ID;
        int target_unit_id = NO_ID;
        int best_dist = 999;

        for (int ci = 0; ci < gs->cities.count; ci++) {
            City *c = &gs->cities.data[ci];
            if (!c->is_active || c->owner != PLAYER_OWNER_ID)
                continue;
            int dist = abs(c->x - u->x) + abs(c->y - u->y);
            if (dist < best_dist) {
                best_dist = dist;
                target_x = c->x;
                target_y = c->y;
            }
        }
        for (int vi = 0; vi < gs->units.count; vi++) {
            Unit *v = &gs->units.data[vi];
            if (!v->is_active || v->owner != PLAYER_OWNER_ID)
                continue;
            int dist = abs(v->x - u->x) + abs(v->y - u->y);
            if (dist < best_dist) {
                best_dist = dist;
                target_x = v->x;
                target_y = v->y;
                target_unit_id = v->id;
            }
        }
        if (target_x == NO_ID)
            continue;
        // If adjacent to a player unit, attack it
        if (target_unit_id != NO_ID && best_dist == 1) {
            event_push(gs, EVENT_AI_ATTACK, f->id,
                "%s attaque votre unite #%d !", f->name, target_unit_id);
            unit_attack(gs, u->id, target_unit_id);
            continue;
        }
        // Otherwise step toward target
        int dx = sign(target_x - u->x);
        int dy = sign(target_y - u->y);
        // Try primary axis first, then secondary
        if (dx != 0 && unit_move(gs, u->id, u->x + dx, u->y))
            continue;
        if (dy != 0 && unit_move(gs, u->id, u->x, u->y + dy))
            continue;
    }
}

void ai_tick(GameState *gs, int faction_idx)
{
    AIFaction *f = &gs->ai_factions.data[faction_idx];
    if (f->is_eliminated)
        return;
    ai_research_tick(gs, f);
    // Reset moves for all faction units
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner != f->id)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        if (tmpl)
            u->moves_left = tmpl->movement;
    }
    // Spawn periodically based on aggression
    int spawn_interval = 11 - f->aggression;
    if (spawn_interval < 2)
        spawn_interval = 2;
    if (gs->current_turn % spawn_interval == 0)
        ai_spawn_unit(gs, faction_idx);
    // Attack if it's time
    if (gs->current_turn >= f->next_attack_turn) {
        ai_try_attack(gs, faction_idx);
        f->next_attack_turn = gs->current_turn + (15 - f->aggression);
        if (f->next_attack_turn <= gs->current_turn)
            f->next_attack_turn = gs->current_turn + 2;
    }
    // Check elimination: faction has no units and no cities
    bool has_presence = false;
    for (int i = 0; i < gs->units.count; i++) {
        if (gs->units.data[i].is_active && gs->units.data[i].owner == f->id) {
            has_presence = true;
            break;
        }
    }
    if (!has_presence) {
        for (int i = 0; i < gs->cities.count; i++) {
            if (gs->cities.data[i].is_active && gs->cities.data[i].owner == f->id) {
                has_presence = true;
                break;
            }
        }
    }
    if (!has_presence)
        f->is_eliminated = true;
}
