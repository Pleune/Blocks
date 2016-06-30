#ifndef UI_H
#define UI_H

#include "textbox.h"

#include <stdint.h>
#include <SDL.h>

typedef struct interface interface_t;

enum interface_flags {
	NIL
};

void interface_static_init();

interface_t *interface_create(enum interface_flags flags);
void interface_destroy(interface_t *interface);

int32_t interface_attach_textbox(interface_t *interface, struct textbox *textbox);

void interface_render(interface_t *interface);
void interface_event_detect(interface_t *interface, SDL_Event *event);

extern uint32_t interface_event_base;

//SDL-like event defines
#define INTERFACE_MOUSECLICKEVENT (interface_event_base)
#define INTERFACE_MOUSEOVEREVENT (interface_event_base+1)
#define INTERFACE_MOUSEOVEREXITEVENT (interface_event_base+2)
#define INTERFACE_NUM_EVENTS 3

#endif
