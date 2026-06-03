#ifndef CONSTANTS_H
#define CONSTANTS_H

// Map
#define MAP_DEFAULT_WIDTH 15
#define MAP_DEFAULT_HEIGHT 15

// Sentinel used everywhere an id/index is absent
#define NO_ID -1

// The player always has owner id 0; AI factions use their faction.id (>= 1)
#define PLAYER_OWNER_ID 0

// Tech tree limits
#define MAX_TECH_PREREQS 4
#define MAX_TECH_UNLOCKS 4
#define MAX_RESEARCH_QUEUE 8

// Rocket victory
#define ROCKET_TOTAL_STAGES 5

// Must equal the number of values in the SpecialAbility enum
#define ABILITY_COUNT 4

// Religion victory: % of non-water tiles that must be converted
#define RELIGION_VICTORY_THRESHOLD 75

// Default game settings
#define DEFAULT_MAX_TURNS 200
#define DEFAULT_NUM_AI 3

// Starting capacity for every dynamic array
#define ARRAY_INITIAL_CAPACITY 8

#endif
