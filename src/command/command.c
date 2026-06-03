#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "command/command.h"
#include "ui/render.h"

Command command_parse(const char *input)
{
    Command cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.args[0] = NO_ID;
    cmd.args[1] = NO_ID;
    cmd.args[2] = NO_ID;
    cmd.args[3] = NO_ID;

    if (!input || input[0] == '\0')
        return cmd;

    // Extract verb (first token)
    int i = 0;
    int v = 0;
    while (input[i] && !isspace((unsigned char)input[i]) && v < 15) {
        cmd.verb[v++] = input[i++];
    }
    cmd.verb[v] = '\0';

    // Skip spaces
    while (input[i] && isspace((unsigned char)input[i]))
        i++;

    // Try to read up to 4 integer args; if a token is not a number, treat it as str_arg
    int arg_idx = 0;
    while (input[i] && arg_idx < 4) {
        // Check if token is a number (possibly negative)
        int is_num = (input[i] == '-' && isdigit((unsigned char)input[i + 1])) || isdigit((unsigned char)input[i]);
        if (is_num) {
            int val = 0;
            int neg = 0;
            if (input[i] == '-') {
                neg = 1;
                i++;
            }
            while (input[i] && isdigit((unsigned char)input[i]))
                val = val * 10 + (input[i++] - '0');
            cmd.args[arg_idx++] = neg ? -val : val;
        } else {
            // Non-numeric token goes into str_arg (only the first one)
            if (cmd.str_arg[0] == '\0') {
                int s = 0;
                while (input[i] && !isspace((unsigned char)input[i]) && s < 31)
                    cmd.str_arg[s++] = input[i++];
                cmd.str_arg[s] = '\0';
            } else {
                // Skip unknown extra tokens
                while (input[i] && !isspace((unsigned char)input[i]))
                    i++;
            }
        }
        while (input[i] && isspace((unsigned char)input[i]))
            i++;
    }
    return cmd;
}

bool command_validate(GameState *gs, Command *cmd)
{
    (void)gs;
    if (cmd->verb[0] == '\0')
        return false;
    const char *valid[] = {
        "move", "attack", "found", "research", "build",
        "found_religion", "info", "tech",
        "next", "save", "load", "help", "quit", NULL
    };
    for (int i = 0; valid[i]; i++) {
        if (strcmp(cmd->verb, valid[i]) == 0)
            return true;
    }
    return false;
}

bool command_read(GameState *gs, Command *out)
{
    char buf[128];
    render_read_input(buf, sizeof(buf));
    if (buf[0] == '\0')
        return false;
    *out = command_parse(buf);
    if (!command_validate(gs, out)) {
        if (out->verb[0] != '\0')
            render_message(gs, "Commande inconnue: '%s'. Tapez 'help'.", out->verb);
        return false;
    }
    return true;
}

void command_print_help(void)
{
    printf("Commandes disponibles:\n");
    printf("  move <unit_id> <x> <y>         Deplacer une unite\n");
    printf("  attack <unit_id> <target_id>   Attaquer une unite ennemie\n");
    printf("  found <nom_ville>              Fonder une ville (avec Settler)\n");
    printf("  research <nom_tech>            Lancer une recherche\n");
    printf("  build <city_id> <unit|0> <id>  Choisir un projet de production\n");
    printf("  found_religion <nom>           Fonder une religion\n");
    printf("  next                           Passer au tour suivant\n");
    printf("  save <fichier>                 Sauvegarder\n");
    printf("  load <fichier>                 Charger\n");
    printf("  help                           Afficher ce message\n");
    printf("  quit                           Quitter\n");
}
