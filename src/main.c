#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "ui/render.h"
#include "civs/civ.h"

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("  --victory science|score|military|religion  (default: score)\n");
    printf("  --turns N     max turns (default: %d)\n", DEFAULT_MAX_TURNS);
    printf("  --ai N        AI factions (default: from difficulty, max %d)\n", MAX_AI_FACTIONS);
}

int main(int argc, char **argv)
{
    
    int cli_turns = DEFAULT_MAX_TURNS;
    int cli_ai = -1; 

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--turns") == 0 && i + 1 < argc)
            cli_turns = atoi(argv[++i]);
        else if (strcmp(argv[i], "--ai") == 0 && i + 1 < argc)
            cli_ai = atoi(argv[++i]);
    }

    render_init();

    bool replay = true;
    while (replay) {
        GameConfig config;
        config.victory_type = VICTORY_SCORE;
        config.max_turns = cli_turns;
        config.map_width = MAP_DEFAULT_WIDTH;
        config.map_height = MAP_DEFAULT_HEIGHT;
        config.num_ai_factions = DEFAULT_NUM_AI;
        config.difficulty = DIFF_MEDIUM;
        config.civ_id = 0;

        int civ_id = 0;
        render_start_menu(&config, &civ_id);

        
        if (cli_ai >= 0)
            config.num_ai_factions = cli_ai;

        GameState gs;
        game_init(&gs, config);
        civ_apply(&gs, civ_id);
        game_run(&gs);
        replay = render_end_screen(&gs);
        game_free(&gs);
    }

    render_cleanup();
    return 0;
}
