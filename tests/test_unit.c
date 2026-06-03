#include "test_runner.h"
#include "helpers.h"
#include "entities/unit.h"

static void t_unit_create_basic(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID); // Settler
    ASSERT_TRUE(uid != NO_ID);

    Unit *u = unit_get(&gs, uid);
    ASSERT_TRUE(u != NULL);
    ASSERT_EQ(u->template_id, 2);
    ASSERT_EQ(u->x, 5);
    ASSERT_EQ(u->y, 5);
    ASSERT_EQ(u->owner, PLAYER_OWNER_ID);
    ASSERT_EQ(u->hp, 5); // max_hp du Settler

    // La case doit referencer l'unite
    ASSERT_EQ(gs.map.grid[5][5].unit_id, uid);

    gs_free_minimal(&gs);
}

static void t_unit_create_case_occupee(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID);
    int uid2 = unit_create(&gs, 0, 5, 5, PLAYER_OWNER_ID); // meme case
    ASSERT_EQ(uid2, NO_ID);

    gs_free_minimal(&gs);
}

static void t_unit_move_adjacent(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID);

    ASSERT_TRUE(unit_move(&gs, uid, 5, 6)); // deplacement d'une case

    Unit *u = unit_get(&gs, uid);
    ASSERT_EQ(u->x, 5);
    ASSERT_EQ(u->y, 6);
    ASSERT_EQ(gs.map.grid[5][5].unit_id, NO_ID); // ancienne case vide
    ASSERT_EQ(gs.map.grid[6][5].unit_id, uid);   // nouvelle case

    gs_free_minimal(&gs);
}

static void t_unit_move_trop_loin(void)
{
    // Deplacement de plus d'une case en une commande = interdit
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID);
    ASSERT_FALSE(unit_move(&gs, uid, 5, 8)); // 3 cases d'un coup

    gs_free_minimal(&gs);
}

static void t_unit_move_sur_eau(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);
    gs.map.grid[6][5].type = TERRAIN_WATER;

    int uid = unit_create(&gs, 2, 5, 5, PLAYER_OWNER_ID); // Settler
    ASSERT_FALSE(unit_move(&gs, uid, 5, 6)); // case eau = bloquee

    gs_free_minimal(&gs);
}

static void t_unit_attack_reduit_hp(void)
{
    GameState gs;
    gs_init_minimal(&gs, 10, 10);

    int att = unit_create(&gs, 0, 5, 5, PLAYER_OWNER_ID);  // Guerrier atk=4
    int def = unit_create(&gs, 0, 5, 6, 1);                // Guerrier def=3, hp=10

    // La tech Guerre est requise pour que unit_attack fonctionne correctement
    // (les templates sont utilises directement, pas de verif tech dans unit_attack)
    unit_attack(&gs, att, def);

    Unit *defender = unit_get(&gs, def);
    // HP doit avoir diminue (atk 4 vs def 3 => 1 degat minimum)
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
