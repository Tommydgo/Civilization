#ifndef EVENT_H
#define EVENT_H

#include "structs.h"

void event_push(GameState *gs, EventType type, int owner, const char *fmt, ...);
void event_clear(GameState *gs);

#endif
