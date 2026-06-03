#ifndef COMMAND_H
#define COMMAND_H

#include "structs.h"

bool command_read(GameState *gs, Command *out);
Command command_parse(const char *input);
bool command_validate(GameState *gs, Command *cmd);
void command_print_help(void);

#endif
