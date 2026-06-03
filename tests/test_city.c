#include "test_runner.h"
#include "helpers.h"
#include "entities/city.h"
#include "entities/unit.h"
#include "tech/tech.h"

// Plaine : prod_yield = 1(base) + 1(terrain) = 2/tour
#define PLAIN_PROD_PER_TURN 2

static void t_city_found_basic(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    ASSERT_TRUE(cid != NO_ID);

    City *c = city_get(&gs, cid);
    ASSERT_TRUE(c != NULL);
    ASSERT_EQ(c->owner, PLAYER_OWNER_ID);
    ASSERT_EQ(c->x, 5);
    ASSERT_EQ(c->y, 5);
    ASSERT_EQ(c->population, 1);
    ASSERT_EQ(c->prod_project, NO_ID);

    // La case de la carte doit referencer la ville
    ASSERT_EQ(gs.map.grid[5][5].city_id, cid);

    gs_free_minimal(&gs);
}

static void t_city_found_sur_eau_impossible(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    gs.map.grid[3][3].type = TERRAIN_WATER;

    int cid = city_found(&gs, 3, 3, PLAYER_OWNER_ID, "Atlantis");
    ASSERT_EQ(cid, NO_ID);

    gs_free_minimal(&gs);
}

// --- Tests build / production ---

static void t_build_settler_sans_tech(void)
{
    // Settler (id=2) n'a pas de tech requise — doit fonctionner immediatement
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    ASSERT_TRUE(city_set_project(&gs, cid, 2, PROD_UNIT));

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->prod_project, 2);
    ASSERT_EQ(c->prod_type, PROD_UNIT);

    gs_free_minimal(&gs);
}

static void t_build_guerrier_sans_tech_impossible(void)
{
    // Guerrier (id=0) requiert tech "Guerre" (id=2) — doit echouer sans la tech
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    ASSERT_FALSE(city_set_project(&gs, cid, 0, PROD_UNIT));

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->prod_project, NO_ID); // projet inchange

    gs_free_minimal(&gs);
}

static void t_build_guerrier_avec_tech(void)
{
    // Apres avoir recherche "Guerre", le Guerrier doit etre buildable
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    gs_give_tech(&gs, PLAYER_OWNER_ID, 2); // tech id=2 = "Guerre"
    ASSERT_TRUE(city_set_project(&gs, cid, 0, PROD_UNIT));

    gs_free_minimal(&gs);
}

static void t_build_production_accumule(void)
{
    // Sur plaine : 2 prod/tour. Apres N ticks, production == N*2
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    city_set_project(&gs, cid, 2, PROD_UNIT); // Settler cost=30

    city_tick(&gs, cid);
    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->production, PLAIN_PROD_PER_TURN);

    city_tick(&gs, cid);
    c = city_get(&gs, cid);
    ASSERT_EQ(c->production, PLAIN_PROD_PER_TURN * 2);

    gs_free_minimal(&gs);
}

static void t_build_settler_se_complete(void)
{
    // Settler cost=30, plaine=2/tour => 15 tours
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    city_set_project(&gs, cid, 2, PROD_UNIT);

    int settler_cost = 30;
    int ticks_needed = settler_cost / PLAIN_PROD_PER_TURN;

    for (int i = 0; i < ticks_needed - 1; i++) {
        city_tick(&gs, cid);
        City *c = city_get(&gs, cid);
        ASSERT_TRUE(c->prod_project != NO_ID); // pas encore fini
    }
    city_tick(&gs, cid); // dernier tour

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->prod_project, NO_ID);   // projet reinitialise
    ASSERT_EQ(c->production, 0);         // compteur remis a zero
    ASSERT_EQ(c->prod_type, PROD_NONE);

    // Une unite Settler doit exister sur la carte
    bool found = false;
    for (int i = 0; i < gs.units.count; i++) {
        if (gs.units.data[i].template_id == 2
                && gs.units.data[i].owner == PLAYER_OWNER_ID
                && gs.units.data[i].is_active)
            found = true;
    }
    ASSERT_TRUE(found);

    gs_free_minimal(&gs);
}

static void t_build_change_projet_reset_production(void)
{
    // Changer de projet remet la production a 0
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    city_set_project(&gs, cid, 2, PROD_UNIT); // Settler

    city_tick(&gs, cid);
    city_tick(&gs, cid); // 4 prod accumules

    gs_give_tech(&gs, PLAYER_OWNER_ID, 2); // debloquer Guerrier
    city_set_project(&gs, cid, 0, PROD_UNIT); // changer vers Guerrier

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->production, 0); // production remise a zero!

    gs_free_minimal(&gs);
}

static void t_build_batiment_deja_construit(void)
{
    // Construire le meme batiment deux fois est impossible
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    gs_give_tech(&gs, PLAYER_OWNER_ID, 0); // Agriculture pour Grenier

    ASSERT_TRUE(city_set_project(&gs, cid, 0, PROD_BUILDING));

    // Simule la completion manuelle du batiment
    City *c = city_get(&gs, cid);
    c->production = 999; // force la completion au prochain tick
    city_tick(&gs, cid);

    // Deuxieme tentative — deja construit
    ASSERT_FALSE(city_set_project(&gs, cid, 0, PROD_BUILDING));

    gs_free_minimal(&gs);
}

static void t_build_ville_inexistante(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    ASSERT_FALSE(city_set_project(&gs, 99, 2, PROD_UNIT));

    gs_free_minimal(&gs);
}

void suite_city(void)
{
    SUITE("Villes et build");
    RUN_TEST("city_found cree une ville valide",            t_city_found_basic);
    RUN_TEST("city_found sur eau = impossible",             t_city_found_sur_eau_impossible);
    RUN_TEST("build Settler sans tech = OK",                t_build_settler_sans_tech);
    RUN_TEST("build Guerrier sans tech = ECHEC",            t_build_guerrier_sans_tech_impossible);
    RUN_TEST("build Guerrier apres Guerre = OK",            t_build_guerrier_avec_tech);
    RUN_TEST("production accumule 2/tour (plaine)",         t_build_production_accumule);
    RUN_TEST("Settler se complete en 15 tours",             t_build_settler_se_complete);
    RUN_TEST("changer projet remet production a 0",         t_build_change_projet_reset_production);
    RUN_TEST("meme batiment deux fois = ECHEC",             t_build_batiment_deja_construit);
    RUN_TEST("build ville inexistante = ECHEC",             t_build_ville_inexistante);
}
