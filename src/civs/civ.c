#include <string.h>
#include "civs/civ.h"
#include "entities/unit.h"
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "world/map.h"

// Tech IDs (from TECH_TREE in tech_tree.c):
// 0=Agriculture 1=Ecriture 2=Guerre 3=Mathematiques
// 4=Chevalerie  5=Theologie 6=Industrie 7=Fusee 8=IA_Generale

const CivTemplate CIV_TEMPLATES[] = {
    {
        0, "Rome", "+1 Guerrier au depart, +1 attaque permanente",
        1, 0, 0, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, false, false, false},
        0  // extra_unit: template 0 = Guerrier
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
        {1, NO_ID},  // free tech: Ecriture (id=1)
        {false, false, false, false},
        NO_ID
    },
    {
        3, "Mongolie", "Guerre offerte au depart, +1 mouvement aux unites",
        0, 1, 0, 0, 0, 0, 0, 0,
        {2, NO_ID},  // free tech: Guerre (id=2)
        {false, false, false, false},
        NO_ID
    },
    {
        4, "Chine", "Frontieres culturelles actives, +5 culture/tour",
        0, 0, 0, 0, 0, 0, 5, 0,
        {NO_ID, NO_ID},
        {false, false, false, true},  // ABILITY_CULTURE_BORDER
        NO_ID
    },
    {
        5, "Azteques", "Religion disponible, +1 Missionnaire au depart",
        0, 0, 0, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {true, false, false, false},  // ABILITY_FOUND_RELIGION
        3  // extra_unit: template 3 = Missionnaire
    },
    {
        6, "Angleterre", "Settlers peuvent traverser l'eau (COLONIZE)",
        0, 0, 0, 0, 0, 0, 0, 0,
        {NO_ID, NO_ID},
        {false, true, false, false},  // ABILITY_COLONIZE
        NO_ID
    },
    {
        7, "Allemagne", "Industrie offerte au depart",
        0, 0, 0, 0, 0, 0, 0, 10,
        {6, NO_ID},  // free tech: Industrie (id=6)
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
    // Copy bonus values to Empire
    gs->player.unit_attack_bonus = civ->unit_attack_bonus;
    gs->player.unit_move_bonus = civ->unit_move_bonus;
    gs->player.unit_defense_bonus = civ->unit_defense_bonus;
    gs->player.city_food_bonus = civ->city_food_bonus;
    gs->player.city_prod_bonus = civ->city_prod_bonus;
    gs->player.science_per_turn_bonus = civ->science_per_turn_bonus;
    gs->player.culture_per_turn_bonus = civ->culture_per_turn_bonus;
    gs->player.gold += civ->start_gold;
    // Starting abilities
    for (int i = 0; i < ABILITY_COUNT; i++) {
        if (civ->start_abilities[i])
            gs->player.abilities[i] = true;
    }
    // Rocket unlocked if ABILITY_ROCKET_PROGRAM starts active
    if (gs->player.abilities[ABILITY_ROCKET_PROGRAM])
        gs->player.rocket.unlocked = true;
    // Free techs
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
    // Extra starting unit at player's first city or starting unit position
    if (civ->extra_unit_template != NO_ID) {
        // Find the player's starting Settler position and spawn there
        for (int i = 0; i < gs->units.count; i++) {
            Unit *u = &gs->units.data[i];
            if (!u->is_active || u->owner != PLAYER_OWNER_ID)
                continue;
            // Try to spawn near this unit
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
    // Apply move bonus to all existing player units
    if (civ->unit_move_bonus != 0) {
        for (int i = 0; i < gs->units.count; i++) {
            Unit *u = &gs->units.data[i];
            if (u->is_active && u->owner == PLAYER_OWNER_ID)
                u->moves_left += civ->unit_move_bonus;
        }
    }
}
