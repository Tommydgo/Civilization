#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include "constants.h"
#include "generic_array.h"

typedef enum {
    TERRAIN_PLAIN,
    TERRAIN_MOUNTAIN,
    TERRAIN_WATER
} TerrainType;

typedef enum {
    ERA_ANCIENT,
    ERA_CLASSICAL,
    ERA_MEDIEVAL,
    ERA_RENAISSANCE,
    ERA_INDUSTRIAL,
    ERA_MODERN,
    ERA_FUTURE
} TechEra;

typedef enum {
    UNLOCK_UNIT,      
    UNLOCK_BUILDING,  
    UNLOCK_ABILITY    
} UnlockType;

typedef enum {
    ABILITY_FOUND_RELIGION,  
    ABILITY_COLONIZE,        
    ABILITY_ROCKET_PROGRAM,  
    ABILITY_CULTURE_BORDER   
} SpecialAbility;

typedef enum {
    ROLE_WARRIOR,
    ROLE_SETTLER,
    ROLE_MISSIONARY,
    ROLE_ROCKET_ENGINEER
} UnitRole;

typedef enum {
    PROD_NONE,
    PROD_UNIT,
    PROD_BUILDING
} ProdProjectType;

typedef enum {
    VICTORY_SCIENCE,
    VICTORY_SCORE,
    VICTORY_MILITARY,
    VICTORY_RELIGION
} VictoryType;

typedef struct {
    int food_mod;
    int prod_mod;
    int defense_bonus; 
    int science_mod;
} TerrainStats;

typedef struct {
    UnlockType type;
    int ref_id; 
} TechUnlock;

typedef struct {
    int id;
    char name[32];
    TechEra era;
    int base_cost;
    int prereq_ids[MAX_TECH_PREREQS]; 
    int prereq_count;
    TechUnlock unlocks[MAX_TECH_UNLOCKS];
    int unlock_count;
    int culture_bonus; 
} TechDef;

typedef struct {
    int tech_id;
    bool researched;
    int progress; 
} TechState;

DEFINE_ARRAY(TechState, TechState)

typedef struct {
    int current_tech_id;            
    int queued_ids[MAX_RESEARCH_QUEUE]; 
    int queue_count;
} ResearchQueue;

typedef struct {
    int id;
    char name[32];
    int prod_cost;
    int food_bonus;
    int prod_bonus;
    int science_bonus;
    int culture_bonus;
    int required_tech_id; 
} BuildingTemplate;

typedef struct {
    int building_id;
    bool is_active;
} CityBuilding;

DEFINE_ARRAY(CityBuilding, CityBuilding)

typedef struct {
    int id;
    char name[32];
    int prod_cost;
    int max_hp;
    int attack;
    int defense;
    int movement;
    UnitRole role;
    int required_tech_id; 
} UnitTemplate;

typedef struct {
    int id;
    int template_id;
    int x;
    int y;
    int owner; 
    int hp;
    int moves_left; 
    bool is_active;
} Unit;

DEFINE_ARRAY(Unit, Unit)

typedef struct {
    int id;
    char name[32];
    int founder_owner;
    int converted_tiles; 
} Religion;

DEFINE_ARRAY(Religion, Religion)

typedef struct {
    int stages_completed; 
    int progress;         
    int stage_cost;       
    bool unlocked;        
} RocketProject;

typedef struct {
    int id;
    char name[32];
    int x;
    int y;
    int owner;
    int population;
    int food;
    int food_cap; 
    int production; 
    
    int prod_project;
    ProdProjectType prod_type;
    int religion_id;     
    int culture_points;  
    CityBuildingArray buildings;
    bool is_active;
} City;

DEFINE_ARRAY(City, City)

typedef struct {
    int x;
    int y;
    TerrainType type;
    int city_id;      
    int unit_id;      
    int religion_id;  
    int culture_owner; 
} Tile;

typedef struct {
    Tile **grid; 
    int width;
    int height;
} Map;

typedef struct {
    int gold;
    int science;
    int gold_per_turn;
    int science_per_turn;
    int culture_points; 
    int score;
    TechStateArray techs; 
    ResearchQueue research;
    bool abilities[ABILITY_COUNT]; 
    RocketProject rocket;
    
    int unit_attack_bonus;
    int unit_move_bonus;
    int unit_defense_bonus;
    int city_food_bonus;
    int city_prod_bonus;
    int science_per_turn_bonus;
    int culture_per_turn_bonus;
} Empire;

typedef struct {
    int id;
    char name[32];
    int aggression; 
    int spawn_x;
    int spawn_y;
    int next_attack_turn;
    int score;
    TechStateArray techs;
    ResearchQueue research;
    bool abilities[ABILITY_COUNT];
    RocketProject rocket;
    bool is_eliminated;
} AIFaction;

DEFINE_ARRAY(AIFaction, AIFaction)

DEFINE_ARRAY(int, Int)

typedef struct {
    VictoryType victory_type;
    int max_turns;
    int map_width;
    int map_height;
    int num_ai_factions;
    int difficulty; 
    int civ_id;     
} GameConfig;

typedef struct {
    bool achieved;
    VictoryType type;
    int winner_owner; 
} VictoryState;

typedef enum {
    EVENT_TECH_DONE,
    EVENT_UNIT_KILLED,
    EVENT_CITY_CAPTURED,
    EVENT_CITY_FOUNDED,
    EVENT_UNIT_CREATED,
    EVENT_AI_SPAWNED,
    EVENT_AI_ATTACK,
    EVENT_ROCKET_STAGE,
} EventType;

typedef struct {
    EventType type;
    int owner; 
    char msg[96];
} GameEvent;

DEFINE_ARRAY(GameEvent, GameEvent)

typedef struct {
    int current_turn;
    GameConfig config;
    Map map;
    Empire player;
    UnitArray units;
    CityArray cities;
    AIFactionArray ai_factions;
    ReligionArray religions;
    VictoryState victory;
    GameEventArray events; 
    bool game_over;
} GameState;

typedef struct {
    char verb[16];
    int args[4];    
    char str_arg[32]; 
} Command;

#endif
