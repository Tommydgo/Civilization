#include <stdio.h>
#include <stdarg.h>
#include "events/event.h"

void event_push(GameState *gs, EventType type, int owner, const char *fmt, ...)
{
    GameEvent ev;
    ev.type = type;
    ev.owner = owner;
    va_list args;
    va_start(args, fmt);
    vsnprintf(ev.msg, sizeof(ev.msg), fmt, args);
    va_end(args);
    GameEventArray_push(&gs->events, ev);
}

void event_clear(GameState *gs)
{
    gs->events.count = 0;
}
