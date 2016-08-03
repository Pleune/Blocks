#include "interface.h"
#include "textbox.h"

#include <stdlib.h>

#include "debug.h"

enum interface_element_type {
	INTERFACE_ELEMENT_TEXTBOX
};

struct interface_element {
	struct interface_element *next;
	void *ptr;
	int32_t uid;

	int *x;
	int *y;
	int *w;
	int *h;

	enum interface_element_type type;
};

struct interface {
	enum interface_flags flags;

	struct interface_element *element_list;
	struct interface_element *element_list_top;

	int32_t next_uid;
};

uint32_t interface_event_base = 0;

void
element_render(struct interface_element *element)
{
	switch(element->type)
	{
	case INTERFACE_ELEMENT_TEXTBOX:
		textbox_render(element->ptr);
		break;
	}
}

void
element_free(struct interface_element *element)
{
	switch(element->type)
	{
	case INTERFACE_ELEMENT_TEXTBOX:
		textbox_destroy(element->ptr);
		break;
	}

	free(element);
}

struct interface_element *
element_get_next(interface_t *interface)
{
	struct interface_element *element;
	if(interface->element_list_top)
	{
		interface->element_list_top->next = malloc(sizeof(struct interface_element));
		element = interface->element_list_top->next;
	} else {
		interface->element_list = malloc(sizeof(struct interface_element));
		element = interface->element_list;
	}

	interface->element_list_top = element;

	element->uid = interface->next_uid;
	++interface->next_uid;

	element->next = 0;

	return element;
}

struct interface_element *
element_getfrom_xy(interface_t *interface, int x, int y)
{
	struct interface_element *working_element = interface->element_list;

	while(working_element)
	{
		int x_ = *(working_element->x);
		int y_ = *(working_element->y);
		int w_ = *(working_element->w);
		int h_ = *(working_element->h);

		if(x>x_ && x < x_+w_ && y>y_ && y < y_+h_)
			return working_element;

		working_element = working_element->next;
	}

	return 0;
}

void
interface_static_init()
{
	interface_event_base = SDL_RegisterEvents(INTERFACE_NUM_EVENTS);
	if(interface_event_base == (uint32_t)-1)
		fail("Could not register interface events with SDL2");
}

interface_t *
interface_create(enum interface_flags flags)
{
	interface_t *ret = malloc(sizeof(struct interface));

	ret->flags = flags;

	ret->element_list = 0;
	ret->element_list_top = 0;

	ret->next_uid = 0;

	return ret;
}

void
interface_destroy(interface_t *interface)
{
	struct interface_element *working_element = interface->element_list;

	while(working_element)
	{
		struct interface_element *this = working_element;
		working_element = working_element->next;
		element_free(this);
	}
	free(interface);
}

int
interface_attach_textbox(interface_t *interface, struct textbox *textbox)
{
	struct interface_element *element = element_get_next(interface);
	element->ptr = textbox;
	element->type = INTERFACE_ELEMENT_TEXTBOX;

	element->x = &textbox->x;
	element->y = &textbox->y;
	element->w = &textbox->w;
	element->h = &textbox->h;

	return element->uid;
}

void
interface_render(interface_t *interface)
{
	struct interface_element *working_element = interface->element_list;

	while(working_element)
	{
		element_render(working_element);
		working_element = working_element->next;
	}
}

void
event_mouseclickevent(int32_t uid)
{
	SDL_Event user_event;
	SDL_memset(&user_event, 0, sizeof(user_event));
	user_event.type = INTERFACE_MOUSECLICKEVENT;
	user_event.user.code = uid;

	SDL_PushEvent(&user_event);
}

void
event_mouseoverevent(int32_t uid)
{
	SDL_Event user_event;
	SDL_memset(&user_event, 0, sizeof(user_event));
	user_event.type = INTERFACE_MOUSEOVEREVENT;
	user_event.user.code = uid;

	SDL_PushEvent(&user_event);
}

void
event_mouseoverexitevent(int32_t uid)
{
	SDL_Event user_event;
	SDL_memset(&user_event, 0, sizeof(user_event));
	user_event.type = INTERFACE_MOUSEOVEREXITEVENT;
	user_event.user.code = uid;

	SDL_PushEvent(&user_event);
}

void
interface_event_detect(interface_t *interface, SDL_Event *event)
{
	struct interface_element *element;
	static int32_t mouse_in_uid = -1;
	switch(event->type)
	{
	case SDL_MOUSEBUTTONDOWN:
		element = element_getfrom_xy(interface, event->button.x, event->button.y);
		if(element)
			event_mouseclickevent(element->uid);
		break;

	case SDL_MOUSEMOTION:
		element = element_getfrom_xy(interface, event->motion.x, event->motion.y);
		if(element)
		{
			if(element->uid == mouse_in_uid)
				break;
			if(mouse_in_uid != -1)
				event_mouseoverexitevent(mouse_in_uid);

			event_mouseoverevent(element->uid);
			mouse_in_uid = element->uid;
		} else {
			if(mouse_in_uid != -1)
				event_mouseoverexitevent(mouse_in_uid);
			mouse_in_uid = -1;
		}

		break;

	default:
		break;
	}
}
