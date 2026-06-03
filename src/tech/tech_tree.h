#ifndef TECH_TREE_H
#define TECH_TREE_H

#include "structs.h"

extern const TechDef TECH_TREE[];
extern const int TECH_TREE_COUNT;

const TechDef *tech_tree_get(int id);
int tech_tree_find(const char *name);
int tech_tree_count(void);
bool tech_tree_era_has_research(GameState *gs, int owner, TechEra era);

#endif
