#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game.h"
#include "world/map.h"
#include "world/tile.h"
#include "entities/unit.h"
#include "entities/city.h"
#include "tech/tech.h"
#include "tech/tech_tree.h"
#include "empire/empire.h"
#include "empire/religion.h"
#include "ai/ai.h"
#include "ai/ai_research.h"
#include "victory/score.h"
#include "victory/victory.h"
#include "ui/render.h"
#include "command/command.h"
#include "saves/save.h"

// ── Spawn helpers ─────────────────────────────────────────────────────────────

// Find a free land tile near (cx, cy), searching outward in a square spiral.
static bool find_land_near(GameState *gs, int cx, int cy, int *ox, int *oy)
{
    for (int r = 0; r <= 7; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (r > 0 && dx != -r && dx != r && dy != -r && dy != r)
                    continue;
                Tile *t = map_get(gs, cx + dx, cy + dy);
                if (t && t->type != TERRAIN_WATER && t->unit_id == NO_ID && t->city_id == NO_ID) {
                    *ox = cx + dx;
                    *oy = cy + dy;
                    return true;
                }
            }
        }
    }
    return false;
}

// ── Init / free ───────────────────────────────────────────────────────────────

void game_init(GameState *gs, GameConfig config)
{
    srand((unsigned)time(NULL));
    memset(gs, 0, sizeof(GameState));
    gs->config = config;
    gs->current_turn = 1;
    gs->game_over = false;
    gs->victory.achieved = false;

    // Init all dynamic top-level arrays
    UnitArray_init(&gs->units);
    CityArray_init(&gs->cities);
    AIFactionArray_init(&gs->ai_factions);
    ReligionArray_init(&gs->religions);

    map_init(gs);
    map_generate(gs);
    empire_init(gs);

    // Push AI faction stubs so ai_faction_init can access them by index
    const char *faction_names[] = {"Barbares", "Huns", "Mongols"};
    int spawn_corners[3][2] = {
        {config.map_width - 3, 2},
        {2, config.map_height - 3},
        {config.map_width - 3, config.map_height - 3}
    };
    int aggressions[] = {5, 8, 3};

    for (int i = 0; i < config.num_ai_factions && i < 3; i++) {
        AIFaction f;
        memset(&f, 0, sizeof(f));
        strncpy(f.name, faction_names[i], 31);
        f.aggression = aggressions[i];
        f.spawn_x = spawn_corners[i][0];
        f.spawn_y = spawn_corners[i][1];
        AIFactionArray_push(&gs->ai_factions, f);
    }
    for (int i = 0; i < gs->ai_factions.count; i++)
        ai_faction_init(gs, i);

    // Place player's starting Settler near top-left
    int sx, sy;
    if (find_land_near(gs, 2, 2, &sx, &sy))
        unit_create(gs, 2, sx, sy, PLAYER_OWNER_ID); // template 2 = Settler
}

void game_free(GameState *gs)
{
    for (int i = 0; i < gs->cities.count; i++)
        CityBuildingArray_free(&gs->cities.data[i].buildings);
    CityArray_free(&gs->cities);
    UnitArray_free(&gs->units);
    for (int i = 0; i < gs->ai_factions.count; i++)
        TechStateArray_free(&gs->ai_factions.data[i].techs);
    AIFactionArray_free(&gs->ai_factions);
    ReligionArray_free(&gs->religions);
    empire_free(gs);
    map_free(gs);
}

// ── Per-turn logic ────────────────────────────────────────────────────────────

void game_tick(GameState *gs)
{
    gs->current_turn++;
    // Reset player unit moves
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner != PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        if (tmpl)
            u->moves_left = tmpl->movement;
    }
    // City production and growth
    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].is_active)
            city_tick(gs, gs->cities.data[i].id);
    }
    // Player resources and culture
    empire_tick(gs);
    // Player research progress
    tech_research_tick(gs, PLAYER_OWNER_ID);
    // AI factions
    for (int i = 0; i < gs->ai_factions.count; i++)
        ai_tick(gs, i);
    // Religion spread
    religion_spread_tick(gs);
    // Score and victory
    score_update_all(gs);
    victory_check(gs);
}

// ── Command dispatch ──────────────────────────────────────────────────────────

static void cmd_move(GameState *gs, Command cmd)
{
    int unit_id = cmd.args[0];
    int tx = cmd.args[1];
    int ty = cmd.args[2];

    if (!unit_move(gs, unit_id, tx, ty))
        render_message("Deplacement impossible.");
}

static void cmd_attack(GameState *gs, Command cmd)
{
    int attacker_id = cmd.args[0];
    int target_id = cmd.args[1];
    Unit *target = unit_get(gs, target_id);

    if (!target) {
        render_message("Cible introuvable.");
        return;
    }
    if (target->owner == PLAYER_OWNER_ID) {
        render_message("Vous ne pouvez pas attaquer vos propres unites.");
        return;
    }
    unit_attack(gs, attacker_id, target_id);
}

static void cmd_found(GameState *gs, Command cmd)
{
    const char *city_name = cmd.str_arg[0] ? cmd.str_arg : "Ville";
    int settler_id = NO_ID;

    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner != PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        if (tmpl && tmpl->role == ROLE_SETTLER) {
            settler_id = u->id;
            break;
        }
    }
    if (settler_id == NO_ID) {
        render_message("Aucun Settler disponible.");
        return;
    }
    Unit *settler = unit_get(gs, settler_id);
    int city_id = city_found(gs, settler->x, settler->y, PLAYER_OWNER_ID, city_name);
    if (city_id == NO_ID) {
        render_message("Impossible de fonder une ville ici.");
        return;
    }
    unit_kill(gs, settler_id);
    render_message("Ville '%s' fondee!", city_name);
}

static void cmd_research(GameState *gs, Command cmd)
{
    int tech_id = tech_tree_find(cmd.str_arg);

    if (tech_id == NO_ID) {
        render_message("Technologie '%s' introuvable.", cmd.str_arg);
        return;
    }
    if (!tech_can_research(gs, PLAYER_OWNER_ID, tech_id)) {
        render_message("Impossible de rechercher '%s' (prerequis manquants).", cmd.str_arg);
        return;
    }
    if (gs->player.research.current_tech_id == NO_ID) {
        gs->player.research.current_tech_id = tech_id;
        render_message("Recherche lancee: %s", cmd.str_arg);
    } else if (gs->player.research.queue_count < MAX_RESEARCH_QUEUE) {
        gs->player.research.queued_ids[gs->player.research.queue_count++] = tech_id;
        render_message("%s ajoutee a la file de recherche.", cmd.str_arg);
    } else {
        render_message("File de recherche pleine.");
    }
}

static void cmd_build(GameState *gs, Command cmd)
{
    int city_id = cmd.args[0];
    int template_id = cmd.args[1];
    ProdProjectType type = PROD_NONE;

    if (strcmp(cmd.str_arg, "unit") == 0)
        type = PROD_UNIT;
    else if (strcmp(cmd.str_arg, "building") == 0)
        type = PROD_BUILDING;
    else {
        render_message("Type invalide: 'unit' ou 'building'.");
        return;
    }
    if (!city_set_project(gs, city_id, template_id, type))
        render_message("Projet de production invalide.");
    else
        render_message("Projet de production defini.");
}

static void cmd_found_religion(GameState *gs, Command cmd)
{
    const char *name = cmd.str_arg[0] ? cmd.str_arg : "Religion";
    int rid = religion_found(gs, PLAYER_OWNER_ID, name);

    if (rid == NO_ID)
        render_message("Impossible de fonder une religion (prerequis ou deja fondee).");
    else
        render_message("Religion '%s' fondee!", name);
}

void game_dispatch(GameState *gs, Command cmd)
{
    if (strcmp(cmd.verb, "move") == 0)
        cmd_move(gs, cmd);
    else if (strcmp(cmd.verb, "attack") == 0)
        cmd_attack(gs, cmd);
    else if (strcmp(cmd.verb, "found") == 0)
        cmd_found(gs, cmd);
    else if (strcmp(cmd.verb, "research") == 0)
        cmd_research(gs, cmd);
    else if (strcmp(cmd.verb, "build") == 0)
        cmd_build(gs, cmd);
    else if (strcmp(cmd.verb, "found_religion") == 0)
        cmd_found_religion(gs, cmd);
    else if (strcmp(cmd.verb, "save") == 0) {
        char path[64];
        snprintf(path, sizeof(path), "%s.civ",
            cmd.str_arg[0] ? cmd.str_arg : "save");
        save_write(gs, path)
            ? render_message("Sauvegarde: %s", path)
            : render_message("Erreur de sauvegarde.");
    } else if (strcmp(cmd.verb, "load") == 0) {
        char path[64];
        snprintf(path, sizeof(path), "%s.civ",
            cmd.str_arg[0] ? cmd.str_arg : "save");
        if (!save_read(gs, path))
            render_message("Erreur de chargement.");
        else
            render_message("Partie chargee.");
    } else if (strcmp(cmd.verb, "help") == 0) {
        command_print_help();
    }
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void game_run(GameState *gs)
{
    while (!gs->game_over) {
        render_turn_start(gs);
        render_map(gs);
        render_status(gs);
        bool turn_done = false;
        while (!turn_done && !gs->game_over) {
            Command cmd;
            if (!command_read(gs, &cmd))
                continue;
            if (strcmp(cmd.verb, "next") == 0) {
                turn_done = true;
            } else if (strcmp(cmd.verb, "quit") == 0) {
                gs->game_over = true;
            } else {
                game_dispatch(gs, cmd);
            }
        }
        if (!gs->game_over)
            game_tick(gs);
    }
    if (gs->victory.achieved) {
        if (gs->victory.winner_owner == PLAYER_OWNER_ID)
            printf("\n*** VICTOIRE! Vous avez gagne! ***\n");
        else
            printf("\n*** DEFAITE! Joueur %d a gagne! ***\n",
                gs->victory.winner_owner);
    } else {
        printf("\nPartie terminee.\n");
    }
}
