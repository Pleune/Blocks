#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <GL/glew.h>

enum font { STANDARD };
#define TEXTBOX_NUM_FONTS 1

typedef struct textbox textbox_t;

void textbox_static_init();
void textbox_static_cleanup();

textbox_t *textbox_create(int x, int y, int w, int h, const char *txt, enum font font);
void textbox_destroy(textbox_t *textbox);

void textbox_set_txt(textbox_t *textbox, const char *txt);
void textbox_set_color(textbox_t *textbox, float r, float g, float b, float a);
void textbox_set_pos(textbox_t *textbox, int x, int y);

void textbox_render(textbox_t *textbox);

#endif
