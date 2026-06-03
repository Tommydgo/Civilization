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
#include "events/event.h"

// ── Spawn helpers ─────────────────────────────────────────────────────────────

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

    UnitArray_init(&gs->units);
    CityArray_init(&gs->cities);
    AIFactionArray_init(&gs->ai_factions);
    ReligionArray_init(&gs->religions);
    GameEventArray_init(&gs->events);

    map_init(gs);
    map_generate(gs);
    empire_init(gs);

    // Up to MAX_AI_FACTIONS factions spread around the map edges
    const char *faction_names[] = {
        "Barbares", "Huns", "Mongols", "Vikings",
        "Perses", "Ottomans", "Celtes"
    };
    int w = config.map_width;
    int h = config.map_height;
    int spawn_pts[7][2] = {
        {w - 3, 3},         {3,     h - 3},     {w - 3, h - 3},
        {w / 2, 3},         {3,     h / 2},      {w - 3, h / 2},
        {w / 2, h - 3}
    };
    // Hard/Extreme: AI starts with one free tech
    int hard_free_tech = 2; // Guerre

    for (int i = 0; i < config.num_ai_factions && i < MAX_AI_FACTIONS; i++) {
        AIFaction f;
        memset(&f, 0, sizeof(f));
        strncpy(f.name, faction_names[i], 31);
        // Scale aggression with difficulty
        f.aggression = 3 + config.difficulty * 2;
        if (f.aggression > 10)
            f.aggression = 10;
        f.spawn_x = spawn_pts[i][0];
        f.spawn_y = spawn_pts[i][1];
        AIFactionArray_push(&gs->ai_factions, f);
    }
    for (int i = 0; i < gs->ai_factions.count; i++) {
        ai_faction_init(gs, i);
        // Hard/Extreme: give AI a free starting tech
        if (config.difficulty >= DIFF_HARD) {
            int faction_id = gs->ai_factions.data[i].id;
            for (int j = 0; j < gs->ai_factions.data[i].techs.count; j++) {
                if (gs->ai_factions.data[i].techs.data[j].tech_id != hard_free_tech)
                    continue;
                const TechDef *def = tech_tree_get(hard_free_tech);
                gs->ai_factions.data[i].techs.data[j].researched = true;
                if (def) {
                    gs->ai_factions.data[i].techs.data[j].progress = def->base_cost;
                    for (int u = 0; u < def->unlock_count; u++)
                        tech_apply_unlock(gs, faction_id, def->unlocks[u]);
                }
                break;
            }
        }
    }

    // Player Settler near center
    int sx, sy;
    int cx = w / 2;
    int cy = h / 2;
    if (find_land_near(gs, cx, cy, &sx, &sy))
        unit_create(gs, 2, sx, sy, PLAYER_OWNER_ID);
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
    GameEventArray_free(&gs->events);
    empire_free(gs);
    map_free(gs);
}

// ── Per-turn logic ────────────────────────────────────────────────────────────

void game_tick(GameState *gs)
{
    event_clear(gs);
    render_info_clear();
    gs->current_turn++;
    for (int i = 0; i < gs->units.count; i++) {
        Unit *u = &gs->units.data[i];
        if (!u->is_active || u->owner != PLAYER_OWNER_ID)
            continue;
        const UnitTemplate *tmpl = unit_template_get(u->template_id);
        if (tmpl)
            u->moves_left = tmpl->movement + gs->player.unit_move_bonus;
    }
    for (int i = 0; i < gs->cities.count; i++) {
        if (gs->cities.data[i].is_active)
            city_tick(gs, gs->cities.data[i].id);
    }
    int culture_before = gs->player.culture_points;
    empire_tick(gs);
    int culture_gained = gs->player.culture_points - culture_before;
    tech_research_tick(gs, PLAYER_OWNER_ID);
    for (int i = 0; i < gs->ai_factions.count; i++)
        ai_tick(gs, i);
    religion_spread_tick(gs);
    score_update_all(gs);
    victory_check(gs);
    event_push(gs, EVENT_TECH_DONE, PLAYER_OWNER_ID,
        "[ Tour %d ]  Or +%d  Sciences +%d  Culture +%d",
        gs->current_turn,
        gs->player.gold_per_turn,
        gs->player.science_per_turn,
        culture_gained);
}

// ── Command dispatch ──────────────────────────────────────────────────────────

static void cmd_move(GameState *gs, Command cmd)
{
    if (cmd.args[0] == NO_ID || cmd.args[1] == NO_ID || cmd.args[2] == NO_ID) {
        render_message(gs, "Usage: move <id_unite> <x> <y>");
        render_message(gs, "Ex:    move 3 26 25  (unite #3 -> (26,25))");
        render_message(gs, "L'id de l'unite est affiche dans le panneau Unites.");
        return;
    }
    if (!unit_move(gs, cmd.args[0], cmd.args[1], cmd.args[2]))
        render_message(gs, "Deplacement impossible (trop loin, obstacle, eau?).");
}

static void cmd_attack(GameState *gs, Command cmd)
{
    if (cmd.args[0] == NO_ID || cmd.args[1] == NO_ID) {
        render_message(gs, "Usage: attack <id_attaquant> <id_cible>");
        render_message(gs, "Ex:    attack 3 7  (votre unite #3 attaque ennemi #7)");
        render_message(gs, "Les ids sont visibles dans le panneau Unites.");
        return;
    }
    Unit *target = unit_get(gs, cmd.args[1]);
    if (!target) {
        render_message(gs, "Cible #%d introuvable.", cmd.args[1]);
        return;
    }
    if (target->owner == PLAYER_OWNER_ID) {
        render_message(gs, "Vous ne pouvez pas attaquer vos propres unites.");
        return;
    }
    unit_attack(gs, cmd.args[0], cmd.args[1]);
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
        render_message(gs, "Aucun Settler disponible.");
        return;
    }
    Unit *settler = unit_get(gs, settler_id);
    int city_id = city_found(gs, settler->x, settler->y, PLAYER_OWNER_ID, city_name);
    if (city_id == NO_ID) {
        render_message(gs, "Impossible de fonder une ville ici.");
        return;
    }
    unit_kill(gs, settler_id);
    render_message(gs, "Ville '%s' fondee !", city_name);
}

static void cmd_research(GameState *gs, Command cmd)
{
    if (cmd.str_arg[0] == '\0') {
        render_message(gs, "Usage: research <nom_tech>");
        render_message(gs, "Ex:    research Agriculture");
        render_message(gs, "Tapez 'tech' pour voir l'arbre technologique.");
        return;
    }
    int tech_id = tech_tree_find(cmd.str_arg);
    if (tech_id == NO_ID) {
        render_message(gs, "Technologie '%s' introuvable. Tapez 'tech'.", cmd.str_arg);
        return;
    }
    if (!tech_can_research(gs, PLAYER_OWNER_ID, tech_id)) {
        render_message(gs, "Prerequis manquants pour '%s'.", cmd.str_arg);
        return;
    }
    if (gs->player.research.current_tech_id == NO_ID) {
        gs->player.research.current_tech_id = tech_id;
        render_message(gs, "Recherche lancee : %s", cmd.str_arg);
    } else if (gs->player.research.queue_count < MAX_RESEARCH_QUEUE) {
        gs->player.research.queued_ids[gs->player.research.queue_count++] = tech_id;
        render_message(gs, "%s ajoutee a la file.", cmd.str_arg);
    } else {
        render_message(gs, "File de recherche pleine.");
    }
}

static void cmd_build_help(void)
{
    render_info_clear();
    render_info_push("=== build : definir la production d'une ville ===");
    render_info_push("");
    render_info_push("Usage: build <id_ville> unit|bld <id_modele>");
    render_info_push(" id_ville : numero visible dans le panneau Villes");
    render_info_push(" unit|bld : type de production (unite ou batiment)");
    render_info_push(" id_modele: numero de l'unite ou du batiment");
    render_info_push("");
    render_info_push("Unites (unit) :");
    render_info_push("  0 Guerrier     20prod  atk:4 def:3 mv:2");
    render_info_push("  1 Cavalier     40prod  atk:6 def:4 mv:3  [tech:Guerre]");
    render_info_push("  2 Settler      30prod  fonde une ville");
    render_info_push("  3 Missionnaire 25prod  repand religion   [tech:Ecriture]");
    render_info_push("  4 Ing.Fusee    50prod  fusee spatiale    [tech:Fusee]");
    render_info_push("Batiments (bld) :");
    render_info_push("  0 Grenier      30prod  +2 nourrit./tour  [tech:Agriculture]");
    render_info_push("  1 Usine        80prod  +3 prod +1 sci    [tech:Industrie]");
    render_info_push("");
    render_info_push("Ex: build 0 unit 0    Guerrier dans ville #0");
    render_info_push("Ex: build 0 bld 0     Grenier  dans ville #0");
    render_info_push("");
    render_info_push("(tapez 'next' pour revenir aux evenements)");
}

static void cmd_build(GameState *gs, Command cmd)
{
    if (cmd.args[0] == NO_ID || cmd.str_arg[0] == '\0' || cmd.args[1] == NO_ID) {
        cmd_build_help();
        return;
    }
    ProdProjectType type = PROD_NONE;
    if (strcmp(cmd.str_arg, "unit") == 0 || strcmp(cmd.str_arg, "u") == 0)
        type = PROD_UNIT;
    else if (strcmp(cmd.str_arg, "building") == 0 || strcmp(cmd.str_arg, "bld") == 0
             || strcmp(cmd.str_arg, "b") == 0)
        type = PROD_BUILDING;
    else {
        cmd_build_help();
        return;
    }
    City *city = city_get(gs, cmd.args[0]);
    if (!city) {
        render_message(gs, "Ville #%d introuvable.", cmd.args[0]);
        return;
    }
    if (!city_can_produce(gs, cmd.args[0], cmd.args[1], type)) {
        if (type == PROD_UNIT) {
            const UnitTemplate *tmpl = unit_template_get(cmd.args[1]);
            if (tmpl && tmpl->required_tech_id != NO_ID
                    && !tech_is_researched(gs, PLAYER_OWNER_ID, tmpl->required_tech_id)) {
                const TechDef *td = tech_tree_get(tmpl->required_tech_id);
                render_message(gs, "Tech requise : '%s' pour produire %s.",
                    td ? td->name : "?", tmpl->name);
                return;
            }
        } else {
            const BuildingTemplate *tmpl = building_template_get(cmd.args[1]);
            if (tmpl && tmpl->required_tech_id != NO_ID
                    && !tech_is_researched(gs, PLAYER_OWNER_ID, tmpl->required_tech_id)) {
                const TechDef *td = tech_tree_get(tmpl->required_tech_id);
                render_message(gs, "Tech requise : '%s' pour construire %s.",
                    td ? td->name : "?", tmpl->name);
                return;
            }
            if (tmpl) {
                render_message(gs, "%s deja construit dans cette ville.", tmpl->name);
                return;
            }
        }
        render_message(gs, "Production impossible. Tapez 'build' pour l'aide.");
        return;
    }
    city_set_project(gs, cmd.args[0], cmd.args[1], type);
    render_message(gs, "Production definie.");
}

static void cmd_found_religion(GameState *gs, Command cmd)
{
    const char *name = cmd.str_arg[0] ? cmd.str_arg : "Religion";
    int rid = religion_found(gs, PLAYER_OWNER_ID, name);
    if (rid == NO_ID)
        render_message(gs, "Impossible de fonder une religion.");
    else
        render_message(gs, "Religion '%s' fondee !", name);
}

static void cmd_info_unit(GameState *gs, Command cmd)
{
    Unit *u = unit_get(gs, cmd.args[0]);
    if (!u) {
        render_message(gs, "Unite #%d introuvable.", cmd.args[0]);
        return;
    }
    const UnitTemplate *tmpl = unit_template_get(u->template_id);
    render_info_clear();
    render_info_push("=== Unite #%d ===", u->id);
    render_info_push("Type     : %s", tmpl ? tmpl->name : "?");
    render_info_push("Position : (%d, %d)", u->x, u->y);
    render_info_push("HP       : %d / %d", u->hp, tmpl ? tmpl->max_hp : 0);
    render_info_push("Attaque  : %d    Defense : %d", tmpl ? tmpl->attack : 0, tmpl ? tmpl->defense : 0);
    render_info_push("Mouvement: %d restants / %d", u->moves_left, tmpl ? tmpl->movement : 0);
    if (u->owner == PLAYER_OWNER_ID)
        render_info_push("Owner    : Vous");
    else
        render_info_push("Owner    : IA #%d", u->owner);
    render_info_push("(Appuyez 'next' pour revenir aux evenements)");
}

static void cmd_info_city(GameState *gs, Command cmd)
{
    City *c = city_get(gs, cmd.args[0]);
    if (!c) {
        render_message(gs, "Ville #%d introuvable.", cmd.args[0]);
        return;
    }
    render_info_clear();
    render_info_push("=== Ville #%d : %s ===", c->id, c->name);
    render_info_push("Position   : (%d, %d)", c->x, c->y);
    render_info_push("Population : %d  (food %d/%d)", c->population, c->food, c->food_cap);
    render_info_push("Culture    : %d pts", c->culture_points);
    if (c->prod_type != PROD_NONE && c->prod_project != NO_ID) {
        int cost = 0;
        const char *pname = "?";
        if (c->prod_type == PROD_UNIT) {
            const UnitTemplate *t = unit_template_get(c->prod_project);
            if (t) { cost = t->prod_cost; pname = t->name; }
        } else {
            const BuildingTemplate *t = building_template_get(c->prod_project);
            if (t) { cost = t->prod_cost; pname = t->name; }
        }
        render_info_push("Production : %s  %d/%d", pname, c->production, cost);
    } else {
        render_info_push("Production : idle");
    }
    render_info_push("Batiments  (%d) :", c->buildings.count);
    for (int i = 0; i < c->buildings.count; i++) {
        const BuildingTemplate *b = building_template_get(c->buildings.data[i].building_id);
        render_info_push("  - %s", b ? b->name : "?");
    }
    render_info_push("(Appuyez 'next' pour revenir aux evenements)");
}

static void cmd_info(GameState *gs, Command cmd)
{
    if (strcmp(cmd.str_arg, "unit") == 0)
        cmd_info_unit(gs, cmd);
    else if (strcmp(cmd.str_arg, "city") == 0)
        cmd_info_city(gs, cmd);
    else
        render_message(gs, "Usage: info unit <id>  ou  info city <id>");
}

static void cmd_tech(GameState *gs, Command cmd)
{
    (void)cmd;
    render_info_clear();
    render_info_push("=== Arbre Technologique ===");
    const char *era_names[] = {
        "Antique", "Classique", "Medieval", "Renaissance",
        "Industriel", "Moderne", "Futur"
    };
    TechEra current_era = ERA_ANCIENT;
    for (int i = 0; i < tech_tree_count(); i++) {
        const TechDef *def = tech_tree_get(i);
        if (!def)
            continue;
        if (def->era != current_era) {
            current_era = def->era;
            render_info_push("--- %s ---", era_names[current_era]);
        }
        bool done = tech_is_researched(gs, PLAYER_OWNER_ID, def->id);
        bool avail = !done && tech_can_research(gs, PLAYER_OWNER_ID, def->id);
        const char *marker = done ? "[v]" : (avail ? "[ ]" : "[x]");
        render_info_push("%s %-16s cout:%3d", marker, def->name, def->base_cost);
    }
    render_info_push("[v]=fait  [ ]=dispo  [x]=bloque");
}

static void cmd_save(GameState *gs, Command cmd)
{
    char path[64];
    snprintf(path, sizeof(path), "%s.civ", cmd.str_arg[0] ? cmd.str_arg : "save");
    if (save_write(gs, path))
        render_message(gs, "Sauvegarde : %s", path);
    else
        render_message(gs, "Erreur de sauvegarde.");
}

static void cmd_load(GameState *gs, Command cmd)
{
    char path[64];
    snprintf(path, sizeof(path), "%s.civ", cmd.str_arg[0] ? cmd.str_arg : "save");
    if (!save_read(gs, path))
        render_message(gs, "Erreur de chargement.");
    else
        render_message(gs, "Partie chargee.");
}

static void cmd_clear(GameState *gs, Command cmd)
{
    (void)cmd;
    event_clear(gs);
    render_info_clear();
}

static void cmd_help(GameState *gs, Command cmd)
{
    (void)gs;
    (void)cmd;
    render_info_clear();
    render_info_push("=== Aide complete ===");
    render_info_push("");
    render_info_push("move <id> <x> <y>      deplacer une unite");
    render_info_push("attack <id> <ennemi>   attaquer (adjacent)");
    render_info_push("found <nom>            fonder ville (Settler)");
    render_info_push("research <tech>        lancer une recherche");
    render_info_push("build                  (tapez 'build' seul)");
    render_info_push("found_religion <nom>   fonder une religion");
    render_info_push("info unit|city <id>    details d'un objet");
    render_info_push("tech                   arbre technologique");
    render_info_push("next                   passer au tour suivant");
    render_info_push("save <nom>             sauvegarder");
    render_info_push("load <nom>             charger");
    render_info_push("quit                   quitter");
    render_info_push("");
    render_info_push("Astuce: entrez une commande incomplete pour");
    render_info_push("        voir un exemple d'utilisation.");
}

typedef void (*CmdFn)(GameState *, Command);

typedef struct {
    const char *verb;
    CmdFn       fn;
} CmdEntry;

static const CmdEntry COMMANDS[] = {
    { "move",           cmd_move },
    { "attack",         cmd_attack },
    { "found",          cmd_found },
    { "research",       cmd_research },
    { "build",          cmd_build },
    { "found_religion", cmd_found_religion },
    { "info",           cmd_info },
    { "tech",           cmd_tech },
    { "save",           cmd_save },
    { "load",           cmd_load },
    { "clear",          cmd_clear },
    { "help",           cmd_help },
    { NULL,             NULL }
};

void game_dispatch(GameState *gs, Command cmd)
{
    int i = 0;

    while (COMMANDS[i].verb) {
        if (strcmp(cmd.verb, COMMANDS[i].verb) == 0) {
            COMMANDS[i].fn(gs, cmd);
            return;
        }
        i++;
    }
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void game_run(GameState *gs)
{
    render_full(gs);
    while (!gs->game_over) {
        Command cmd;
        if (!command_read(gs, &cmd))
            continue;
        if (strcmp(cmd.verb, "next") == 0) {
            game_tick(gs);
            render_full(gs);
        } else if (strcmp(cmd.verb, "quit") == 0) {
            gs->game_over = true;
        } else {
            game_dispatch(gs, cmd);
            render_full(gs);
        }
    }
    render_full(gs);
    if (gs->victory.achieved) {
        if (gs->victory.winner_owner == PLAYER_OWNER_ID)
            render_info_push("*** VICTOIRE ! Vous avez gagne ! ***");
        else
            render_info_push("*** DEFAITE ! Joueur %d a gagne ! ***", gs->victory.winner_owner);
        render_full(gs);
    }
}
