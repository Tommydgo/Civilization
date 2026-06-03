#include "test_runner.h"
#include "helpers.h"
#include "entities/city.h"
#include "entities/unit.h"
#include "tech/tech.h"

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

static void t_build_settler_sans_tech(void)
{
    
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
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    ASSERT_FALSE(city_set_project(&gs, cid, 0, PROD_UNIT));

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->prod_project, NO_ID); 

    gs_free_minimal(&gs);
}

static void t_build_guerrier_avec_tech(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");

    gs_give_tech(&gs, PLAYER_OWNER_ID, 2); 
    ASSERT_TRUE(city_set_project(&gs, cid, 0, PROD_UNIT));

    gs_free_minimal(&gs);
}

static void t_build_production_accumule(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    city_set_project(&gs, cid, 2, PROD_UNIT); 

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
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    city_set_project(&gs, cid, 2, PROD_UNIT);

    int settler_cost = 30;
    int ticks_needed = settler_cost / PLAIN_PROD_PER_TURN;

    for (int i = 0; i < ticks_needed - 1; i++) {
        city_tick(&gs, cid);
        City *c = city_get(&gs, cid);
        ASSERT_TRUE(c->prod_project != NO_ID); 
    }
    city_tick(&gs, cid); 

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->prod_project, NO_ID);   
    ASSERT_EQ(c->production, 0);         
    ASSERT_EQ(c->prod_type, PROD_NONE);

    
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
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    city_set_project(&gs, cid, 2, PROD_UNIT); 

    city_tick(&gs, cid);
    city_tick(&gs, cid); 

    gs_give_tech(&gs, PLAYER_OWNER_ID, 2); 
    city_set_project(&gs, cid, 0, PROD_UNIT); 

    City *c = city_get(&gs, cid);
    ASSERT_EQ(c->production, 0); 

    gs_free_minimal(&gs);
}

static void t_build_batiment_deja_construit(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    int cid = city_found(&gs, 5, 5, PLAYER_OWNER_ID, "Roma");
    gs_give_tech(&gs, PLAYER_OWNER_ID, 0); 

    ASSERT_TRUE(city_set_project(&gs, cid, 0, PROD_BUILDING));

    
    City *c = city_get(&gs, cid);
    c->production = 999; 
    city_tick(&gs, cid);

    
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
