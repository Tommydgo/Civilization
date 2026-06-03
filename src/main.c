#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("  --victory science|score|military|religion  (default: score)\n");
    printf("  --turns N     max turns (default: %d)\n", DEFAULT_MAX_TURNS);
    printf("  --ai N        number of AI factions (default: %d, max 3)\n", DEFAULT_NUM_AI);
}

static VictoryType parse_victory(const char *s)
{
    if (strcmp(s, "science") == 0)
        return VICTORY_SCIENCE;
    if (strcmp(s, "military") == 0)
        return VICTORY_MILITARY;
    if (strcmp(s, "religion") == 0)
        return VICTORY_RELIGION;
    return VICTORY_SCORE;
}

int main(int argc, char **argv)
{
    GameConfig config;
    config.victory_type = VICTORY_SCORE;
    config.max_turns = DEFAULT_MAX_TURNS;
    config.map_width = MAP_DEFAULT_WIDTH;
    config.map_height = MAP_DEFAULT_HEIGHT;
    config.num_ai_factions = DEFAULT_NUM_AI;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--victory") == 0 && i + 1 < argc) {
            config.victory_type = parse_victory(argv[++i]);
        } else if (strcmp(argv[i], "--turns") == 0 && i + 1 < argc) {
            config.max_turns = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ai") == 0 && i + 1 < argc) {
            config.num_ai_factions = atoi(argv[++i]);
            if (config.num_ai_factions > 3)
                config.num_ai_factions = 3;
            if (config.num_ai_factions < 0)
                config.num_ai_factions = 0;
        }
    }

    GameState gs;
    game_init(&gs, config);
    game_run(&gs);
    game_free(&gs);
    return 0;
}
