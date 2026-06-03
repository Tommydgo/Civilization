#include <string.h>
#include "tech/tech_tree.h"
#include "empire/empire.h"

const TechDef TECH_TREE[] = {
    
    {
        0, "Agriculture", ERA_ANCIENT, 20,
        {NO_ID, NO_ID, NO_ID, NO_ID}, 0,
        {{UNLOCK_BUILDING, 0}}, 1,    
        0
    },
    {
        1, "Ecriture", ERA_ANCIENT, 30,
        {NO_ID, NO_ID, NO_ID, NO_ID}, 0,
        {{UNLOCK_ABILITY, ABILITY_FOUND_RELIGION}}, 1,
        0
    },
    {
        2, "Guerre", ERA_ANCIENT, 25,
        {NO_ID, NO_ID, NO_ID, NO_ID}, 0,
        {{UNLOCK_UNIT, 0}}, 1,        
        0
    },
    
    {
        3, "Mathematiques", ERA_CLASSICAL, 60,
        {1, NO_ID, NO_ID, NO_ID}, 1, 
        {}, 0,
        2                            
    },
    {
        4, "Chevalerie", ERA_CLASSICAL, 70,
        {2, NO_ID, NO_ID, NO_ID}, 1, 
        {{UNLOCK_UNIT, 1}}, 1,       
        0
    },
    
    {
        5, "Theologie", ERA_MEDIEVAL, 100,
        {1, 3, NO_ID, NO_ID}, 2,     
        {{UNLOCK_ABILITY, ABILITY_CULTURE_BORDER}}, 1,
        0
    },
    
    {
        6, "Industrie", ERA_INDUSTRIAL, 200,
        {NO_ID, NO_ID, NO_ID, NO_ID}, 0,
        {{UNLOCK_BUILDING, 1}}, 1,   
        0
    },
    
    {
        7, "Fusee", ERA_MODERN, 350,
        {6, 3, NO_ID, NO_ID}, 2,     
        {{UNLOCK_ABILITY, ABILITY_ROCKET_PROGRAM}}, 1,
        0
    },
    
    {
        8, "IA Generale", ERA_FUTURE, 500,
        {NO_ID, NO_ID, NO_ID, NO_ID}, 0,
        {}, 0,
        10                           
    },
};

const int TECH_TREE_COUNT = 9;

const TechDef *tech_tree_get(int id)
{
    if (id < 0 || id >= TECH_TREE_COUNT)
        return NULL;
    return &TECH_TREE[id];
}

int tech_tree_find(const char *name)
{
    for (int i = 0; i < TECH_TREE_COUNT; i++) {
        if (strcmp(TECH_TREE[i].name, name) == 0)
            return i;
    }
    return NO_ID;
}

int tech_tree_count(void)
{
    return TECH_TREE_COUNT;
}

bool tech_tree_era_has_research(GameState *gs, int owner, TechEra era)
{
    TechStateArray *techs = owner_techs(gs, owner);
    if (!techs)
        return false;
    for (int i = 0; i < techs->count; i++) {
        if (!techs->data[i].researched)
            continue;
        const TechDef *def = tech_tree_get(techs->data[i].tech_id);
        if (def && def->era == era)
            return true;
    }
    return false;
}
