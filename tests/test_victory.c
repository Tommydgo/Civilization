#include "test_runner.h"
#include "helpers.h"
#include "entities/city.h"
#include "entities/unit.h"
#include "victory/victory.h"
#include "ai/ai.h"
#include "tech/tech.h"

static void t_military_pas_de_factions(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    ASSERT_FALSE(victory_check_military(&gs));

    gs_free_minimal(&gs);
}

static void t_military_ia_encore_en_vie(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    AIFaction f;
    memset(&f, 0, sizeof(f));
    f.id = 1;
    f.is_eliminated = false;
    tech_init_owner(&gs, 1);
    AIFactionArray_push(&gs.ai_factions, f);
    unit_create(&gs, 0, 1, 1, 1); 

    ASSERT_FALSE(victory_check_military(&gs));

    TechStateArray_free(&gs.ai_factions.data[0].techs);
    gs_free_minimal(&gs);
}

static void t_military_toutes_factions_eliminées(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    AIFaction f;
    memset(&f, 0, sizeof(f));
    f.id = 1;
    f.is_eliminated = true;
    tech_init_owner(&gs, 1);
    AIFactionArray_push(&gs.ai_factions, f);

    ASSERT_TRUE(victory_check_military(&gs));

    TechStateArray_free(&gs.ai_factions.data[0].techs);
    gs_free_minimal(&gs);
}

static void t_military_pas_de_ville_joueur(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    AIFaction f;
    memset(&f, 0, sizeof(f));
    f.id = 1;
    f.is_eliminated = true;
    tech_init_owner(&gs, 1);
    AIFactionArray_push(&gs.ai_factions, f);

    ASSERT_FALSE(victory_check_military(&gs));

    TechStateArray_free(&gs.ai_factions.data[0].techs);
    gs_free_minimal(&gs);
}

static void t_score_pas_avant_max_tours(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    gs.config.max_turns = 200;
    gs.current_turn = 100;

    ASSERT_FALSE(victory_check_score(&gs));

    gs.current_turn = 200;
    ASSERT_TRUE(victory_check_score(&gs));

    gs_free_minimal(&gs);
}

static void t_science_sans_fusee(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    ASSERT_FALSE(victory_check_science(&gs));

    gs_free_minimal(&gs);
}

static void t_science_fusee_complete(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    gs.player.rocket.unlocked = true;
    gs.player.rocket.stages_completed = ROCKET_TOTAL_STAGES;

    ASSERT_TRUE(victory_check_science(&gs));

    gs_free_minimal(&gs);
}

static void t_victory_check_ne_se_declenche_pas_a_tour_2(void)
{
    
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    AIFaction f;
    memset(&f, 0, sizeof(f));
    f.id = 1;
    f.is_eliminated = false;
    tech_init_owner(&gs, 1);
    AIFactionArray_push(&gs.ai_factions, f);
    unit_create(&gs, 0, 1, 1, 1);

    city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    gs.current_turn = 2;

    victory_check(&gs);
    ASSERT_FALSE(gs.victory.achieved);

    TechStateArray_free(&gs.ai_factions.data[0].techs);
    gs_free_minimal(&gs);
}

void suite_victory(void)
{
    SUITE("Conditions de victoire");
    RUN_TEST("militaire : 0 factions = jamais",            t_military_pas_de_factions);
    RUN_TEST("militaire : IA encore en vie = non",         t_military_ia_encore_en_vie);
    RUN_TEST("militaire : toutes IA eliminées = oui",      t_military_toutes_factions_eliminées);
    RUN_TEST("militaire : IA eliminée sans ville = non",   t_military_pas_de_ville_joueur);
    RUN_TEST("score : avant max_tours = non",              t_score_pas_avant_max_tours);
    RUN_TEST("science : sans fusee = non",                 t_science_sans_fusee);
    RUN_TEST("science : fusee complete = oui",             t_science_fusee_complete);
    RUN_TEST("regression : pas de victoire au tour 2",     t_victory_check_ne_se_declenche_pas_a_tour_2);
}
