#ifndef CITY_H
#define CITY_H

#include "structs.h"

extern const BuildingTemplate BUILDING_TEMPLATES[];
extern const int BUILDING_TEMPLATE_COUNT;

int city_found(GameState *gs, int x, int y, int owner, const char *name);
void city_tick(GameState *gs, int city_id);
bool city_set_project(GameState *gs, int city_id, int project_id, ProdProjectType type);
bool city_can_produce(GameState *gs, int city_id, int project_id, ProdProjectType type);
const BuildingTemplate *building_template_get(int id);
City *city_get(GameState *gs, int city_id);
void city_reset_id_counter(GameState *gs);

#endif
