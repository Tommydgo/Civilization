#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "saves/save.h"
#include "entities/unit.h"
#include "entities/city.h"
#include "world/map.h"
#include "tech/tech.h"

#define SAVE_MAGIC 0x43495631u

static void write_empire_fixed(FILE *f, Empire *e)
{
    fwrite(&e->gold, sizeof(int), 1, f);
    fwrite(&e->science, sizeof(int), 1, f);
    fwrite(&e->gold_per_turn, sizeof(int), 1, f);
    fwrite(&e->science_per_turn, sizeof(int), 1, f);
    fwrite(&e->culture_points, sizeof(int), 1, f);
    fwrite(&e->score, sizeof(int), 1, f);
    fwrite(&e->research, sizeof(ResearchQueue), 1, f);
    fwrite(e->abilities, sizeof(bool), ABILITY_COUNT, f);
    fwrite(&e->rocket, sizeof(RocketProject), 1, f);
    fwrite(&e->techs.count, sizeof(int), 1, f);
    fwrite(e->techs.data, sizeof(TechState), e->techs.count, f);
}

static void write_city(FILE *f, City *c)
{
    fwrite(&c->id, sizeof(int), 1, f);
    fwrite(c->name, sizeof(char), 32, f);
    fwrite(&c->x, sizeof(int), 1, f);
    fwrite(&c->y, sizeof(int), 1, f);
    fwrite(&c->owner, sizeof(int), 1, f);
    fwrite(&c->population, sizeof(int), 1, f);
    fwrite(&c->food, sizeof(int), 1, f);
    fwrite(&c->food_cap, sizeof(int), 1, f);
    fwrite(&c->production, sizeof(int), 1, f);
    fwrite(&c->prod_project, sizeof(int), 1, f);
    fwrite(&c->prod_type, sizeof(ProdProjectType), 1, f);
    fwrite(&c->religion_id, sizeof(int), 1, f);
    fwrite(&c->culture_points, sizeof(int), 1, f);
    fwrite(&c->is_active, sizeof(bool), 1, f);
    fwrite(&c->buildings.count, sizeof(int), 1, f);
    fwrite(c->buildings.data, sizeof(CityBuilding), c->buildings.count, f);
}

static void write_faction(FILE *f, AIFaction *fa)
{
    fwrite(&fa->id, sizeof(int), 1, f);
    fwrite(fa->name, sizeof(char), 32, f);
    fwrite(&fa->aggression, sizeof(int), 1, f);
    fwrite(&fa->spawn_x, sizeof(int), 1, f);
    fwrite(&fa->spawn_y, sizeof(int), 1, f);
    fwrite(&fa->next_attack_turn, sizeof(int), 1, f);
    fwrite(&fa->score, sizeof(int), 1, f);
    fwrite(&fa->is_eliminated, sizeof(bool), 1, f);
    fwrite(&fa->research, sizeof(ResearchQueue), 1, f);
    fwrite(fa->abilities, sizeof(bool), ABILITY_COUNT, f);
    fwrite(&fa->rocket, sizeof(RocketProject), 1, f);
    fwrite(&fa->techs.count, sizeof(int), 1, f);
    fwrite(fa->techs.data, sizeof(TechState), fa->techs.count, f);
}

bool save_write(GameState *gs, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    unsigned int magic = SAVE_MAGIC;
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&gs->current_turn, sizeof(int), 1, f);
    fwrite(&gs->config, sizeof(GameConfig), 1, f);
    fwrite(&gs->victory, sizeof(VictoryState), 1, f);
    fwrite(&gs->game_over, sizeof(bool), 1, f);
    
    fwrite(&gs->map.width, sizeof(int), 1, f);
    fwrite(&gs->map.height, sizeof(int), 1, f);
    for (int y = 0; y < gs->map.height; y++)
        fwrite(gs->map.grid[y], sizeof(Tile), gs->map.width, f);
    
    write_empire_fixed(f, &gs->player);
    
    fwrite(&gs->units.count, sizeof(int), 1, f);
    fwrite(gs->units.data, sizeof(Unit), gs->units.count, f);
    
    fwrite(&gs->cities.count, sizeof(int), 1, f);
    for (int i = 0; i < gs->cities.count; i++)
        write_city(f, &gs->cities.data[i]);
    
    fwrite(&gs->ai_factions.count, sizeof(int), 1, f);
    for (int i = 0; i < gs->ai_factions.count; i++)
        write_faction(f, &gs->ai_factions.data[i]);
    
    fwrite(&gs->religions.count, sizeof(int), 1, f);
    fwrite(gs->religions.data, sizeof(Religion), gs->religions.count, f);
    fclose(f);
    return true;
}

static void read_empire_fixed(FILE *f, Empire *e)
{
    fread(&e->gold, sizeof(int), 1, f);
    fread(&e->science, sizeof(int), 1, f);
    fread(&e->gold_per_turn, sizeof(int), 1, f);
    fread(&e->science_per_turn, sizeof(int), 1, f);
    fread(&e->culture_points, sizeof(int), 1, f);
    fread(&e->score, sizeof(int), 1, f);
    fread(&e->research, sizeof(ResearchQueue), 1, f);
    fread(e->abilities, sizeof(bool), ABILITY_COUNT, f);
    fread(&e->rocket, sizeof(RocketProject), 1, f);
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    TechStateArray_init(&e->techs);
    for (int i = 0; i < count; i++) {
        TechState ts;
        fread(&ts, sizeof(TechState), 1, f);
        TechStateArray_push(&e->techs, ts);
    }
}

static void read_city(FILE *f, City *c)
{
    fread(&c->id, sizeof(int), 1, f);
    fread(c->name, sizeof(char), 32, f);
    fread(&c->x, sizeof(int), 1, f);
    fread(&c->y, sizeof(int), 1, f);
    fread(&c->owner, sizeof(int), 1, f);
    fread(&c->population, sizeof(int), 1, f);
    fread(&c->food, sizeof(int), 1, f);
    fread(&c->food_cap, sizeof(int), 1, f);
    fread(&c->production, sizeof(int), 1, f);
    fread(&c->prod_project, sizeof(int), 1, f);
    fread(&c->prod_type, sizeof(ProdProjectType), 1, f);
    fread(&c->religion_id, sizeof(int), 1, f);
    fread(&c->culture_points, sizeof(int), 1, f);
    fread(&c->is_active, sizeof(bool), 1, f);
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    CityBuildingArray_init(&c->buildings);
    for (int i = 0; i < count; i++) {
        CityBuilding b;
        fread(&b, sizeof(CityBuilding), 1, f);
        CityBuildingArray_push(&c->buildings, b);
    }
}

static void read_faction(FILE *f, AIFaction *fa)
{
    fread(&fa->id, sizeof(int), 1, f);
    fread(fa->name, sizeof(char), 32, f);
    fread(&fa->aggression, sizeof(int), 1, f);
    fread(&fa->spawn_x, sizeof(int), 1, f);
    fread(&fa->spawn_y, sizeof(int), 1, f);
    fread(&fa->next_attack_turn, sizeof(int), 1, f);
    fread(&fa->score, sizeof(int), 1, f);
    fread(&fa->is_eliminated, sizeof(bool), 1, f);
    fread(&fa->research, sizeof(ResearchQueue), 1, f);
    fread(fa->abilities, sizeof(bool), ABILITY_COUNT, f);
    fread(&fa->rocket, sizeof(RocketProject), 1, f);
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    TechStateArray_init(&fa->techs);
    for (int i = 0; i < count; i++) {
        TechState ts;
        fread(&ts, sizeof(TechState), 1, f);
        TechStateArray_push(&fa->techs, ts);
    }
}

bool save_read(GameState *gs, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    unsigned int magic = 0;
    fread(&magic, sizeof(magic), 1, f);
    if (magic != SAVE_MAGIC) {
        fclose(f);
        return false;
    }
    
    for (int i = 0; i < gs->cities.count; i++)
        CityBuildingArray_free(&gs->cities.data[i].buildings);
    CityArray_free(&gs->cities);
    UnitArray_free(&gs->units);
    for (int i = 0; i < gs->ai_factions.count; i++)
        TechStateArray_free(&gs->ai_factions.data[i].techs);
    AIFactionArray_free(&gs->ai_factions);
    ReligionArray_free(&gs->religions);
    TechStateArray_free(&gs->player.techs);
    map_free(gs);
    
    fread(&gs->current_turn, sizeof(int), 1, f);
    fread(&gs->config, sizeof(GameConfig), 1, f);
    fread(&gs->victory, sizeof(VictoryState), 1, f);
    fread(&gs->game_over, sizeof(bool), 1, f);
    
    int w = 0;
    int h = 0;
    fread(&w, sizeof(int), 1, f);
    fread(&h, sizeof(int), 1, f);
    gs->config.map_width = w;
    gs->config.map_height = h;
    map_init(gs);
    for (int y = 0; y < h; y++)
        fread(gs->map.grid[y], sizeof(Tile), w, f);
    
    read_empire_fixed(f, &gs->player);
    
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    UnitArray_init(&gs->units);
    for (int i = 0; i < count; i++) {
        Unit u;
        fread(&u, sizeof(Unit), 1, f);
        UnitArray_push(&gs->units, u);
    }
    
    fread(&count, sizeof(int), 1, f);
    CityArray_init(&gs->cities);
    for (int i = 0; i < count; i++) {
        City c;
        read_city(f, &c);
        CityArray_push(&gs->cities, c);
    }
    
    fread(&count, sizeof(int), 1, f);
    AIFactionArray_init(&gs->ai_factions);
    for (int i = 0; i < count; i++) {
        AIFaction fa;
        read_faction(f, &fa);
        AIFactionArray_push(&gs->ai_factions, fa);
    }
    
    fread(&count, sizeof(int), 1, f);
    ReligionArray_init(&gs->religions);
    for (int i = 0; i < count; i++) {
        Religion r;
        fread(&r, sizeof(Religion), 1, f);
        ReligionArray_push(&gs->religions, r);
    }
    fclose(f);
    unit_reset_id_counter(gs);
    city_reset_id_counter(gs);
    return true;
}

void save_list(const char *dir)
{
    DIR *d = opendir(dir ? dir : ".");
    if (!d) {
        printf("Impossible d'ouvrir le repertoire.\n");
        return;
    }
    struct dirent *entry;
    printf("Sauvegardes disponibles:\n");
    while ((entry = readdir(d)) != NULL) {
        int len = (int)strlen(entry->d_name);
        if (len > 4 && strcmp(entry->d_name + len - 4, ".civ") == 0)
            printf("  %s\n", entry->d_name);
    }
    closedir(d);
}
