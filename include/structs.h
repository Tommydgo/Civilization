#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>
#include "constants.h"
#include "generic_array.h"

// ── ENUMS ────────────────────────────────────────────────────────────────────

typedef enum {
    TERRAIN_PLAIN,
    TERRAIN_MOUNTAIN,
    TERRAIN_WATER
} TerrainType;

// Chronological eras; a tech of era N requires at least one researched tech of era N-1
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
    UNLOCK_UNIT,      // makes UNIT_TEMPLATES[ref_id] available for production
    UNLOCK_BUILDING,  // makes BUILDING_TEMPLATES[ref_id] available in cities
    UNLOCK_ABILITY    // sets abilities[ref_id] = true in Empire or AIFaction
} UnlockType;

// ABILITY_COUNT in constants.h must stay equal to the number of values here
typedef enum {
    ABILITY_FOUND_RELIGION,  // allows founding one religion (one-shot)
    ABILITY_COLONIZE,        // allows Settler units to cross water
    ABILITY_ROCKET_PROGRAM,  // unlocks the RocketProject for the owner
    ABILITY_CULTURE_BORDER   // cities expand their cultural influence each turn
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

// ── TERRAIN LOOKUP ───────────────────────────────────────────────────────────

// Indexed by TerrainType; defined as const TerrainStats TERRAIN_STATS[3] in map.c
typedef struct {
    int food_mod;
    int prod_mod;
    int defense_bonus; // added to the defender's effective defense on combat
    int science_mod;
} TerrainStats;

// ── TECH SYSTEM ──────────────────────────────────────────────────────────────

// One effect produced by researching a technology
typedef struct {
    UnlockType type;
    int ref_id; // index in UNIT_TEMPLATES[], BUILDING_TEMPLATES[], or SpecialAbility value
} TechUnlock;

// Immutable blueprint; never modified during a session
// Defined as const TechDef TECH_TREE[] in tech_tree.c
typedef struct {
    int id;
    char name[32];
    TechEra era;
    int base_cost;
    int prereq_ids[MAX_TECH_PREREQS]; // unused slots filled with NO_ID
    int prereq_count;
    TechUnlock unlocks[MAX_TECH_UNLOCKS];
    int unlock_count;
    int culture_bonus; // added to empire.culture_points each turn once researched
} TechDef;

// Per-owner research state for one technology
typedef struct {
    int tech_id;
    bool researched;
    int progress; // accumulated science toward base_cost; only meaningful for current_tech_id
} TechState;

DEFINE_ARRAY(TechState, TechState)

// Research queue for an empire or AI faction
typedef struct {
    int current_tech_id;            // tech being researched this turn; NO_ID if none
    int queued_ids[MAX_RESEARCH_QUEUE]; // planned future techs; [0] is next after current
    int queue_count;
} ResearchQueue;

// ── BUILDINGS ────────────────────────────────────────────────────────────────

// Immutable blueprint; defined as const BuildingTemplate BUILDING_TEMPLATES[] in city.c
typedef struct {
    int id;
    char name[32];
    int prod_cost;
    int food_bonus;
    int prod_bonus;
    int science_bonus;
    int culture_bonus;
    int required_tech_id; // NO_ID if buildable without research
} BuildingTemplate;

// One building instance inside a specific city
typedef struct {
    int building_id;
    bool is_active;
} CityBuilding;

DEFINE_ARRAY(CityBuilding, CityBuilding)

// ── UNITS ────────────────────────────────────────────────────────────────────

// Immutable blueprint; defined as const UnitTemplate UNIT_TEMPLATES[] in unit.c
typedef struct {
    int id;
    char name[32];
    int prod_cost;
    int max_hp;
    int attack;
    int defense;
    int movement;
    UnitRole role;
    int required_tech_id; // NO_ID if trainable without research
} UnitTemplate;

// Live unit instance on the map
typedef struct {
    int id;
    int template_id;
    int x;
    int y;
    int owner; // PLAYER_OWNER_ID or a faction id; NO_ID if unclaimed
    int hp;
    int moves_left; // reset to UnitTemplate.movement at turn start
    bool is_active;
} Unit;

DEFINE_ARRAY(Unit, Unit)

// ── RELIGION ─────────────────────────────────────────────────────────────────

typedef struct {
    int id;
    char name[32];
    int founder_owner;
    int converted_tiles; // refreshed each turn by religion.c
} Religion;

DEFINE_ARRAY(Religion, Religion)

// ── ROCKET PROJECT ───────────────────────────────────────────────────────────

typedef struct {
    int stages_completed; // 0 to ROCKET_TOTAL_STAGES
    int progress;         // production accumulated toward current stage
    int stage_cost;       // production required per stage
    bool unlocked;        // true only after ABILITY_ROCKET_PROGRAM is acquired
} RocketProject;

// ── CITY ─────────────────────────────────────────────────────────────────────

typedef struct {
    int id;
    char name[32];
    int x;
    int y;
    int owner;
    int population;
    int food;
    int food_cap; // food needed to grow population by 1
    int production; // accumulated toward current prod_project
    // prod_project is an index into UNIT_TEMPLATES[] or BUILDING_TEMPLATES[] depending on prod_type
    int prod_project;
    ProdProjectType prod_type;
    int religion_id;     // dominant religion in this city; NO_ID if none
    int culture_points;  // cumulative total; drives culture_owner of surrounding tiles
    CityBuildingArray buildings;
    bool is_active;
} City;

DEFINE_ARRAY(City, City)

// ── MAP ──────────────────────────────────────────────────────────────────────

typedef struct {
    int x;
    int y;
    TerrainType type;
    int city_id;      // NO_ID if no city on this tile
    int unit_id;      // NO_ID if no unit on this tile
    int religion_id;  // NO_ID if not converted
    int culture_owner; // NO_ID = neutral; otherwise the owner controlling this tile culturally
} Tile;

typedef struct {
    Tile **grid; // accessed as grid[y][x]
    int width;
    int height;
} Map;

// ── EMPIRE (PLAYER) ──────────────────────────────────────────────────────────

typedef struct {
    int gold;
    int science;
    int gold_per_turn;
    int science_per_turn;
    int culture_points; // empire total; sum of all city culture_points
    int score;
    TechStateArray techs; // one entry per tech in TECH_TREE[]
    ResearchQueue research;
    bool abilities[ABILITY_COUNT]; // indexed by SpecialAbility; true = unlocked
    RocketProject rocket;
} Empire;

// ── AI FACTION ───────────────────────────────────────────────────────────────

typedef struct {
    int id;
    char name[32];
    int aggression; // 0-10; high = prefers military techs and attacks sooner
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

// Dynamic int list — used for computed tech id sets (valid options, prereqs, etc.)
DEFINE_ARRAY(int, Int)

// ── GAME CONFIGURATION ───────────────────────────────────────────────────────

typedef struct {
    VictoryType victory_type;
    int max_turns;
    int map_width;
    int map_height;
    int num_ai_factions;
} GameConfig;

// ── VICTORY ──────────────────────────────────────────────────────────────────

typedef struct {
    bool achieved;
    VictoryType type;
    int winner_owner; // PLAYER_OWNER_ID or faction id
} VictoryState;

// ── GAME STATE ───────────────────────────────────────────────────────────────

// Single root passed as GameState * to every function in the game.
// No subsystem owns a separate global; all mutable state lives here.
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
    bool game_over;
} GameState;

// ── COMMAND ──────────────────────────────────────────────────────────────────

// Parsed representation of one line of player input
typedef struct {
    char verb[16];
    int args[4];    // integer arguments (coordinates, ids, amounts)
    char str_arg[32]; // string argument (tech name, city name, save slot)
} Command;

#endif
