#include "ai/ai_research.h"
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "empire/empire.h"

bool ai_tech_is_military(int tech_id)
{
    const TechDef *def = tech_tree_get(tech_id);
    if (!def)
        return false;
    for (int i = 0; i < def->unlock_count; i++) {
        if (def->unlocks[i].type == UNLOCK_UNIT)
            return true;
    }
    return false;
}

int ai_select_next_tech(GameState *gs, AIFaction *faction)
{
    int best_id = NO_ID;
    int best_score = -9999;

    for (int i = 0; i < tech_tree_count(); i++) {
        int tech_id = TECH_TREE[i].id;
        if (!tech_can_research(gs, faction->id, tech_id))
            continue;
        const TechDef *def = tech_tree_get(tech_id);
        if (!def)
            continue;
        int score = 0;
        
        if (faction->aggression >= 6 && ai_tech_is_military(tech_id))
            score += 10;
        
        if (faction->aggression < 6 && def->culture_bonus > 0)
            score += 5;
        
        score -= def->base_cost / 10;
        if (score > best_score) {
            best_score = score;
            best_id = tech_id;
        }
    }
    return best_id;
}

void ai_research_tick(GameState *gs, AIFaction *faction)
{
    
    if (faction->research.current_tech_id == NO_ID) {
        int next = ai_select_next_tech(gs, faction);
        if (next != NO_ID)
            faction->research.current_tech_id = next;
    }
    tech_research_tick(gs, faction->id);
}
