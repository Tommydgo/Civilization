#ifndef CIV_H
#define CIV_H

#include "structs.h"

typedef struct {
    int id;
    char name[32];
    char description[80];
    int unit_attack_bonus;
    int unit_move_bonus;
    int unit_defense_bonus;
    int city_food_bonus;
    int city_prod_bonus;
    int science_per_turn_bonus;
    int culture_per_turn_bonus;
    int start_gold;
    int free_tech_ids[2];           
    bool start_abilities[ABILITY_COUNT];
    int extra_unit_template;        
} CivTemplate;

extern const CivTemplate CIV_TEMPLATES[];
extern const int CIV_COUNT;

void civ_apply(GameState *gs, int civ_id);
const CivTemplate *civ_get(int id);

#endif
