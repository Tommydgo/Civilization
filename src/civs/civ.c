#include <string.h>
#include "civs/civ.h"
#include "entities/unit.h"
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "world/map.h"

const CivTemplate CIV_TEMPLATES[] = {
    {
        0, "Rome", "+1 Guerrier au depart, +1 attaque permanente",
        1, 0, 0, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, false, false, false},
        0  
    },
    {
        1, "Egypte", "+1 production dans toutes les villes",
        0, 0, 0, 0, 1, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, false, false, false},
        NO_ID
    },
    {
        2, "Grece", "Ecriture offerte au depart, +2 science/tour",
        0, 0, 0, 0, 0, 2, 0, 0,
        {1, NO_ID},  
        {false, false, false, false},
        NO_ID
    },
    {
        3, "Mongolie", "Guerre offerte au depart, +1 mouvement aux unites",
        0, 1, 0, 0, 0, 0, 0, 0,
        {2, NO_ID},  
        {false, false, false, false},
        NO_ID
    },
    {
        4, "Chine", "Frontieres culturelles actives, +5 culture/tour",
        0, 0, 0, 0, 0, 0, 5, 0,
        {NO_ID, NO_ID},
        {false, false, false, true},  
        NO_ID
    },
    {
        5, "Azteques", "Religion disponible, +1 Missionnaire au depart",
        0, 0, 0, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {true, false, false, false},  
        3  
    },
    {
        6, "Angleterre", "Settlers peuvent traverser l'eau (COLONIZE)",
        0, 0, 0, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, true, false, false},  
        NO_ID
    },
    {
        7, "Allemagne", "Industrie offerte au depart",
        0, 0, 0, 0, 0, 0, 0, 10,
        {6, NO_ID},  
        {false, false, false, false},
        NO_ID
    },
    {
        8, "Japon", "+2 attaque et +1 defense pour toutes les unites",
        2, 0, 1, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, false, false, false},
        NO_ID
    },
    {
        9, "Inde", "+2 nourriture dans toutes les villes",
        0, 0, 0, 2, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, false, false, false},
        NO_ID
    },
};

const int CIV_COUNT = 10;

const CivTemplate *civ_get(int id)
{
    if (id < 0 || id >= CIV_COUNT)
        return NULL;
    return &CIV_TEMPLATES[id];
}

void civ_apply(GameState *gs, int civ_id)
{
    const CivTemplate *civ = civ_get(civ_id);
    if (!civ)
        return;
    
    gs->player.unit_attack_bonus = civ->unit_attack_bonus;
    gs->player.unit_move_bonus = civ->unit_move_bonus;
    gs->player.unit_defense_bonus = civ->unit_defense_bonus;
    gs->player.city_food_bonus = civ->city_food_bonus;
    gs->player.city_prod_bonus = civ->city_prod_bonus;
    gs->player.science_per_turn_bonus = civ->science_per_turn_bonus;
    gs->player.culture_per_turn_bonus = civ->culture_per_turn_bonus;
    gs->player.gold += civ->start_gold;
    
    for (int i = 0; i < ABILITY_COUNT; i++) {
        if (civ->start_abilities[i])
            gs->player.abilities[i] = true;
    }
    
    if (gs->player.abilities[ABILITY_ROCKET_PROGRAM])
        gs->player.rocket.unlocked = true;
    
    for (int i = 0; i < 2; i++) {
        int tid = civ->free_tech_ids[i];
        if (tid == NO_ID)
            continue;
        for (int j = 0; j < gs->player.techs.count; j++) {
            if (gs->player.techs.data[j].tech_id != tid)
                continue;
            if (gs->player.techs.data[j].researched)
                break;
            const TechDef *def = tech_tree_get(tid);
            gs->player.techs.data[j].researched = true;
            if (def) {
                gs->player.techs.data[j].progress = def->base_cost;
                for (int u = 0; u < def->unlock_count; u++)
                    tech_apply_unlock(gs, PLAYER_OWNER_ID, def->unlocks[u]);
            }
            break;
        }
    }
    
    if (civ->extra_unit_template != NO_ID) {
        
        for (int i = 0; i < gs->units.count; i++) {
            Unit *u = &gs->units.data[i];
            if (!u->is_active || u->owner != PLAYER_OWNER_ID)
                continue;
            
            int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
            int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
            for (int d = 0; d < 8; d++) {
                int nx = u->x + dx[d];
                int ny = u->y + dy[d];
                Tile *t = map_get(gs, nx, ny);
                if (t && t->type != TERRAIN_WATER && t->unit_id == NO_ID) {
                    unit_create(gs, civ->extra_unit_template, nx, ny, PLAYER_OWNER_ID);
                    goto done_extra_unit;
                }
            }
            break;
        }
        done_extra_unit:;
    }
    
    if (civ->unit_move_bonus != 0) {
        for (int i = 0; i < gs->units.count; i++) {
            Unit *u = &gs->units.data[i];
            if (u->is_active && u->owner == PLAYER_OWNER_ID)
                u->moves_left += civ->unit_move_bonus;
        }
    }
}
