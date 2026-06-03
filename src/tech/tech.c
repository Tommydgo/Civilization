#include <string.h>
#include <stdio.h>
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "empire/empire.h"
#include "events/event.h"

// Returns the closest non-empty era strictly before target_era, or ERA_ANCIENT.
static TechEra prev_populated_era(TechEra target_era)
{
    for (int e = (int)target_era - 1; e >= ERA_ANCIENT; e--) {
        for (int i = 0; i < tech_tree_count(); i++) {
            if (TECH_TREE[i].era == (TechEra)e)
                return (TechEra)e;
        }
    }
    return ERA_ANCIENT;
}

void tech_init_owner(GameState *gs, int owner)
{
    TechStateArray *arr = owner_techs(gs, owner);
    if (!arr)
        return;
    TechStateArray_init(arr);
    for (int i = 0; i < tech_tree_count(); i++) {
        TechState ts;
        ts.tech_id = TECH_TREE[i].id;
        ts.researched = false;
        ts.progress = 0;
        TechStateArray_push(arr, ts);
    }
}

bool tech_is_researched(GameState *gs, int owner, int tech_id)
{
    TechStateArray *arr = owner_techs(gs, owner);
    if (!arr)
        return false;
    for (int i = 0; i < arr->count; i++) {
        if (arr->data[i].tech_id == tech_id)
            return arr->data[i].researched;
    }
    return false;
}

bool tech_can_research(GameState *gs, int owner, int tech_id)
{
    const TechDef *def = tech_tree_get(tech_id);
    if (!def)
        return false;
    if (tech_is_researched(gs, owner, tech_id))
        return false;
    // Era gate: must have at least one tech in the closest prior populated era
    if (def->era > ERA_ANCIENT) {
        TechEra prev = prev_populated_era(def->era);
        if (!tech_tree_era_has_research(gs, owner, prev))
            return false;
    }
    // Explicit prereqs
    for (int i = 0; i < def->prereq_count; i++) {
        if (!tech_is_researched(gs, owner, def->prereq_ids[i]))
            return false;
    }
    return true;
}

void tech_apply_unlock(GameState *gs, int owner, TechUnlock unlock)
{
    if (unlock.type == UNLOCK_ABILITY) {
        bool *abilities = owner_abilities(gs, owner);
        if (abilities && unlock.ref_id >= 0 && unlock.ref_id < ABILITY_COUNT)
            abilities[unlock.ref_id] = true;
        // ABILITY_ROCKET_PROGRAM also unlocks the rocket project
        if (unlock.ref_id == ABILITY_ROCKET_PROGRAM) {
            RocketProject *rocket = owner_rocket(gs, owner);
            if (rocket)
                rocket->unlocked = true;
        }
    }
    // UNLOCK_UNIT / UNLOCK_BUILDING: availability is derived at runtime from
    // tech_is_researched, so no state change needed here.
}

void tech_research_tick(GameState *gs, int owner)
{
    ResearchQueue *q = owner_research(gs, owner);
    TechStateArray *techs = owner_techs(gs, owner);
    if (!q || !techs)
        return;
    if (q->current_tech_id == NO_ID) {
        // Auto-dequeue next if available
        if (q->queue_count > 0) {
            q->current_tech_id = q->queued_ids[0];
            for (int i = 1; i < q->queue_count; i++)
                q->queued_ids[i - 1] = q->queued_ids[i];
            q->queue_count--;
        } else {
            return;
        }
    }
    // Find the TechState for the current tech
    int science_per_turn = 0;
    if (owner == PLAYER_OWNER_ID)
        science_per_turn = gs->player.science_per_turn;
    else {
        for (int i = 0; i < gs->ai_factions.count; i++) {
            if (gs->ai_factions.data[i].id == owner) {
                // AI science per turn: base 2 + 1 per researched tech
                science_per_turn = 2 + gs->ai_factions.data[i].techs.count / 3;
                break;
            }
        }
    }
    for (int i = 0; i < techs->count; i++) {
        if (techs->data[i].tech_id != q->current_tech_id)
            continue;
        if (techs->data[i].researched) {
            q->current_tech_id = NO_ID;
            break;
        }
        techs->data[i].progress += science_per_turn;
        const TechDef *def = tech_tree_get(q->current_tech_id);
        if (!def)
            break;
        if (techs->data[i].progress >= def->base_cost) {
            techs->data[i].researched = true;
            techs->data[i].progress = def->base_cost;
            for (int u = 0; u < def->unlock_count; u++)
                tech_apply_unlock(gs, owner, def->unlocks[u]);
            if (owner == PLAYER_OWNER_ID)
                event_push(gs, EVENT_TECH_DONE, owner,
                    "Recherche terminee : %s !", def->name);
            else
                event_push(gs, EVENT_TECH_DONE, owner,
                    "IA #%d a recherche : %s", owner, def->name);
            q->current_tech_id = NO_ID;
            // Dequeue next
            if (q->queue_count > 0) {
                q->current_tech_id = q->queued_ids[0];
                for (int j = 1; j < q->queue_count; j++)
                    q->queued_ids[j - 1] = q->queued_ids[j];
                q->queue_count--;
            }
        }
        break;
    }
}
