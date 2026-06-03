#include "test_runner.h"
#include "helpers.h"
#include "entities/unit.h"

static void t_unit_create_basic(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID); 
    ASSERT_TRUE(uid != NO_ID);

    Unit *u = unit_get(&gs, uid);
    ASSERT_TRUE(u != NULL);
    ASSERT_EQ(u->template_id, 2);
    ASSERT_EQ(u->x, 5);
    ASSERT_EQ(u->y, 5);
    ASSERT_EQ(u->owner, PLAYER_OWNER_ID);
    ASSERT_EQ(u->hp, 5); 

    
    ASSERT_EQ(gs.map.grid[5][5].unit_id, uid);

    gs_free_minimal(&gs);
}

static void t_unit_create_case_occupee(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID);
    int uid2 = unit_create(&gs, 0, 5, 5, PLAYER_OWNER_ID); 
    ASSERT_EQ(uid2, NO_ID);

    gs_free_minimal(&gs);
}

static void t_unit_move_adjacent(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID);

    ASSERT_TRUE(unit_move(&gs, uid, 5, 6)); 

    Unit *u = unit_get(&gs, uid);
    ASSERT_EQ(u->x, 5);
    ASSERT_EQ(u->y, 6);
    ASSERT_EQ(gs.map.grid[5][5].unit_id, NO_ID); 
    ASSERT_EQ(gs.map.grid[6][5].unit_id, uid);   

    gs_free_minimal(&gs);
}

static void t_unit_move_trop_loin(void)
{
    
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID);
    ASSERT_FALSE(unit_move(&gs, uid, 5, 8)); 

    gs_free_minimal(&gs);
}

static void t_unit_move_sur_eau(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    gs.map.grid[6][5].type = TERRAIN_WATER;

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID); 
    ASSERT_FALSE(unit_move(&gs, uid, 5, 6)); 

    gs_free_minimal(&gs);
}

static void t_unit_attack_reduit_hp(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int att = unit_create(&gs, 0, 5, 5, PLAYER_OWNER_ID);  
    int def = unit_create(&gs, 0, 5, 6, 1);                

    
    
    unit_attack(&gs, att, def);

    Unit *defender = unit_get(&gs, def);
    
    ASSERT_TRUE(defender == NULL || defender->hp < 10);

    gs_free_minimal(&gs);
}

void suite_unit(void)
{
    SUITE("Unites");
    RUN_TEST("unit_create place l'unite sur la carte",    t_unit_create_basic);
    RUN_TEST("unit_create case occupee = echec",          t_unit_create_case_occupee);
    RUN_TEST("unit_move d'une case adjacente",            t_unit_move_adjacent);
    RUN_TEST("unit_move de plusieurs cases = interdit",   t_unit_move_trop_loin);
    RUN_TEST("unit_move sur eau = interdit",              t_unit_move_sur_eau);
    RUN_TEST("unit_attack reduit les HP",                 t_unit_attack_reduit_hp);
}
