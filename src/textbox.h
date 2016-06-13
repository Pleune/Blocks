#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <stdint.h>
#include <SDL.h>

typedef struct textbox textbox_t;

enum textbox_font {
	TEXTBOX_FONT_ROBOTO_REGULAR,
	TEXTBOX_FONT_ROBOTO_MEDIUM,
	TEXTBOX_FONT_ROBOTO_ITALIC,
	TEXTBOX_FONT_ROBOTO_BOLD,
	TEXTBOX_FONT_ROBOTO_THIN,
	TEXTBOX_FONT_ROBOTO_LIGHT,
	TEXTBOX_FONT_ROBOTO_BLACK,
	TEXTBOX_FONT_ROBOTO_BLACKITALIC,
	TEXTBOX_FONT_ROBOTO_LIGHTITALIC,
	TEXTBOX_FONT_ROBOTO_THINITALIC,
	TEXTBOX_FONT_ROBOTO_BOLDITALIC,
	TEXTBOX_FONT_ROBOTO_MEDIUMITALIC,

	TEXTBOX_FONT_ROBOTOCONDENSED_REGULAR,
	TEXTBOX_FONT_ROBOTOCONDENSED_ITALIC,
	TEXTBOX_FONT_ROBOTOCONDENSED_BOLD,
	TEXTBOX_FONT_ROBOTOCONDENSED_LIGHT,
	TEXTBOX_FONT_ROBOTOCONDENSED_BOLDITALIC,
	TEXTBOX_FONT_ROBOTOCONDENSED_LIGHTITALIC
};

enum textbox_font_size {
	TEXTBOX_FONT_SIZE_SMALL,
	TEXTBOX_FONT_SIZE_MEDIUM,
	TEXTBOX_FONT_SIZE_LARGE,
	TEXTBOX_FONT_SIZE_HUGE
};

enum textbox_flags {
	TEXTBOX_FLAG_CENTER_H = 2,
	TEXTBOX_FLAG_CENTER_V = 4
};

#define TEXTBOX_NUM_FONTS 18
#define TEXTBOX_NUM_SIZES 4

extern const SDL_Color textbox_colors[];
#define TEXTBOX_COLOR_WHITE (&textbox_colors[0])
#define TEXTBOX_COLOR_BLACK (&textbox_colors[1])
#define TEXTBOX_COLOR_RED (&textbox_colors[2])
#define TEXTBOX_COLOR_ORANGE (&textbox_colors[3])
#define TEXTBOX_COLOR_YELLOW (&textbox_colors[4])
#define TEXTBOX_COLOR_GREEN (&textbox_colors[5])
#define TEXTBOX_COLOR_BLUE (&textbox_colors[6])
#define TEXTBOX_COLOR_PURPLE (&textbox_colors[7])

void textbox_static_init();
void textbox_static_cleanup();

textbox_t *textbox_create(
	int x, int y, int w, int h,
	const char *txt,
	const SDL_Color *color,
	enum textbox_font textbox_font,
	enum textbox_font_size size,
	enum textbox_flags flags);

void textbox_destroy(textbox_t *textbox);

void textbox_set_txt(textbox_t *textbox, const char *txt);
void textbox_set_color(textbox_t *textbox, float r, float g, float b, float a);
void textbox_set_pos(textbox_t *textbox, int x, int y);

void textbox_render(textbox_t *textbox);

#endif
