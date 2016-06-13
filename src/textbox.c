#include "textbox.h"

#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

#include "state.h"
#include "debug.h"
#include "gl.h"

struct font_info_ttf {
	const char *name;
	const char *path;
};

struct textbox {
	int w;
	int h;
	int x;
	int y;

	enum textbox_font textbox_font;
	enum textbox_font_size size;
	enum textbox_flags flags;
	SDL_Color color;

   	GLuint vertices_buff;
	GLuint texture;


	char *txt;
};

static struct font_info_ttf ttf_info[TEXTBOX_NUM_FONTS] = {
	[TEXTBOX_FONT_ROBOTO_REGULAR] = {
		"Roboto-Regular",
		"resources/fonts/Roboto/Roboto-Regular.ttf"},
	[TEXTBOX_FONT_ROBOTO_MEDIUM] = {
		"Roboto-Medium",
		"resources/fonts/Roboto/Roboto-Medium.ttf"},
	[TEXTBOX_FONT_ROBOTO_ITALIC] = {
		"Roboto-Italic",
		"resources/fonts/Roboto/Roboto-Italic.ttf"},
	[TEXTBOX_FONT_ROBOTO_BOLD] = {
		"Roboto-Bold",
		"resources/fonts/Roboto/Roboto-Bold.ttf"},
	[TEXTBOX_FONT_ROBOTO_THIN] = {
		"Roboto-Thin",
		"resources/fonts/Roboto/Roboto-Thin.ttf"},
	[TEXTBOX_FONT_ROBOTO_LIGHT] = {
		"Roboto-Light",
		"resources/fonts/Roboto/Roboto-Light.ttf"},
	[TEXTBOX_FONT_ROBOTO_BLACK] = {
		"Roboto-Black",
		"resources/fonts/Roboto/Roboto-Black.ttf"},
	[TEXTBOX_FONT_ROBOTO_BLACKITALIC] = {
		"Roboto-BlackItalic",
		"resources/fonts/Roboto/Roboto-BlackItalic.ttf"},
	[TEXTBOX_FONT_ROBOTO_LIGHTITALIC] = {
		"Roboto-LightItalic",
		"resources/fonts/Roboto/Roboto-LightItalic.ttf"},
	[TEXTBOX_FONT_ROBOTO_THINITALIC] = {
		"Roboto-ThinItalic",
		"resources/fonts/Roboto/Roboto-ThinItalic.ttf"},
	[TEXTBOX_FONT_ROBOTO_BOLDITALIC] = {
		"Roboto-BoldItalic",
		"resources/fonts/Roboto/Roboto-BoldItalic.ttf"},
	[TEXTBOX_FONT_ROBOTO_MEDIUMITALIC] = {
		"Roboto-MediumItalic",
		"resources/fonts/Roboto/Roboto-MediumItalic.ttf"},


	[TEXTBOX_FONT_ROBOTOCONDENSED_REGULAR] = {
		"RobotoCondensed-Regular",
		"resources/fonts/Roboto/RobotoCondensed-Regular.ttf"},
	[TEXTBOX_FONT_ROBOTOCONDENSED_ITALIC] = {
		"RobotoCondensed-Italic",
		"resources/fonts/Roboto/RobotoCondensed-Italic.ttf"},
	[TEXTBOX_FONT_ROBOTOCONDENSED_BOLD] = {
		"RobotoCondensed-Bold",
		"resources/fonts/Roboto/RobotoCondensed-Bold.ttf"},
	[TEXTBOX_FONT_ROBOTOCONDENSED_LIGHT] = {
		"RobotoCondensed-Light",
		"resources/fonts/Roboto/RobotoCondensed-Light.ttf"},
	[TEXTBOX_FONT_ROBOTOCONDENSED_BOLDITALIC] = {
		"RobotoCondensed-BoldItalic",
		"resources/fonts/Roboto/RobotoCondensed-BoldItalic.ttf"},
	[TEXTBOX_FONT_ROBOTOCONDENSED_LIGHTITALIC] = {
		"RobotoCondensed-LightItalic",
		"resources/fonts/Roboto/RobotoCondensed-LightItalic.ttf"}
};

static int size_lookup[TEXTBOX_NUM_SIZES] = {
	[TEXTBOX_FONT_SIZE_SMALL] = 10,
	[TEXTBOX_FONT_SIZE_MEDIUM] = 18,
	[TEXTBOX_FONT_SIZE_LARGE] = 28,
	[TEXTBOX_FONT_SIZE_HUGE] = 60,
};

const SDL_Color textbox_colors[] = {
	{255, 255, 255, 255},
	{  0,   0,   0, 255},
	{255,   0,   0, 255},
	{255, 125,   0, 255},
	{255, 255,   0, 255},
	{  0, 255,   0, 255},
	{  0,   0, 255, 255},
	{125,   0, 255, 255}
};

static GLuint glprogram;
static GLuint uniform_texture;
static GLuint uniform_window_size;
static GLuint uniform_offset;

const static char *shader_vertex = "\
#version 100\n\
precision mediump float;\n\
attribute vec2 pos;\n\
attribute vec2 texcoord_vert;\n\
varying vec2 texcoord_frag;\n\
uniform vec2 window_size;\n\
uniform vec2 offset;\n\
void main() {\n\
	texcoord_frag = texcoord_vert;\n\
    vec2 p = (pos + offset)/window_size;\n\
    p.y = 1.0 - p.y;\n\
    p = p * vec2(2,2) - vec2(1,1);\n\
	gl_Position = vec4(p, 0.0, 1.0);\n\
}\
";

const static char *shader_fragment = "\
#version 100\n\
precision mediump float;\n\
varying vec2 texcoord_frag;\n\
uniform sampler2D texture;\n\
void main()\n\
{\n\
    gl_FragColor = texture2D(texture, texcoord_frag);\n\
}\
";

void
textbox_static_init()
{
	gl_program_load_str(&glprogram, shader_vertex, shader_fragment);
	uniform_texture = glGetUniformLocation(glprogram, "texture");
	uniform_window_size = glGetUniformLocation(glprogram, "window_size");
	uniform_offset = glGetUniformLocation(glprogram, "offset");
}

void
textbox_static_cleanup()
{
}

textbox_t *
textbox_create(
	int x, int y, int w, int h,
	const char* txt,
	const SDL_Color *color,
	enum textbox_font textbox_font,
	enum textbox_font_size size,
	enum textbox_flags flags)
{
	textbox_t *textbox = malloc(sizeof(textbox_t));

	textbox->x = x;
	textbox->y = y;
	textbox->w = w;
	textbox->h = h;

	textbox->textbox_font = textbox_font;
	textbox->size = size;

	if(color)
		textbox->color = *color;
	else
		textbox->color = *TEXTBOX_COLOR_WHITE;

	textbox->flags = flags;

	glGenBuffers(1, &textbox->vertices_buff);
	glGenTextures(1, &textbox->texture);

	textbox_set_txt(textbox, txt);

	return textbox;
}

void
textbox_destroy(struct textbox* textbox)
{
	glDeleteBuffers(1, &textbox->vertices_buff);
	glDeleteTextures(1, &textbox->texture);
	free(textbox);
}

void
textbox_set_txt(textbox_t *textbox, const char *txt)
{
	if(txt != textbox->txt)
	{
		size_t txt_size = strlen(txt) + 1;
		textbox->txt = malloc(txt_size);
		memcpy(textbox->txt, txt, txt_size);
	}

	char *file = calloc(1024, sizeof(char));
	strncat(file, state_basepath_get(), 1024);
	strncat(file, ttf_info[textbox->textbox_font].path, 1024 - strlen(file));

	TTF_Font *font_sdl = TTF_OpenFont(file, size_lookup[textbox->size]);

	if(!font_sdl)
		fail("Unable to load textbox_font: %s", file);

	SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font_sdl, txt, textbox->color, textbox->w);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textbox->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	size_t texture_size = surface->w * surface->h;

	size_t i;
	for(i=0; i<texture_size; ++i)
	{
		uint32_t color = ((uint32_t *)(surface->pixels))[i];
		uint32_t tmp = (color << 8) | (color >> 24);

		((uint32_t *)(surface->pixels))[i] = tmp;
	}

	if(textbox->w < surface->w)
		warn("Textbox: string \"%s\" wider (by %ipx) than textbox", textbox->txt, surface->w - textbox->h);

	if(textbox->h < surface->h)
		warn("Textbox: string \"%s\" taller (by %ipx) than textbox", textbox->txt, surface->h - textbox->h);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h,
				 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, surface->pixels);

	struct {
		GLfloat x;
		GLfloat y;
	} offset = { 0, 0 };

	if(textbox->flags & TEXTBOX_FLAG_CENTER_H)
		offset.x = (textbox->w - surface->w) / 2.0f;

	if(textbox->flags & TEXTBOX_FLAG_CENTER_V)
		offset.y = (textbox->h - surface->h) / 2.0f;

	GLfloat vertices_data[] = {
		offset.x, offset.y, 0, 0,
		offset.x, offset.y + surface->h, 0, 1,
		offset.x + surface->w, offset.y, 1, 0,

		offset.x + surface->w, offset.y + surface->h, 1, 1,
		offset.x + surface->w, offset.y, 1, 0,
		offset.x, offset.y + surface->h, 0, 1
	};
	glBindBuffer(GL_ARRAY_BUFFER, textbox->vertices_buff);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices_data, GL_STATIC_DRAW);

	SDL_FreeSurface(surface);
	TTF_CloseFont(font_sdl);
}

void
textbox_set_color(textbox_t* textbox, float r, float g, float b, float a)
{
	textbox->color.r = r * 255;
	textbox->color.g = g * 255;
	textbox->color.b = b * 255;
	textbox->color.a = a * 255;

	textbox_set_txt(textbox, textbox->txt);
}

void
textbox_set_pos(textbox_t* textbox, int x, int y)
{
	textbox->x = x;
	textbox->y = y;
}

void
textbox_render(textbox_t* textbox)
{
	glDisable(GL_DEPTH_TEST);
	glUseProgram(glprogram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textbox->texture);
	glUniform1i(uniform_texture, 0);

	int window_w, window_h;
	state_window_get_size(&window_w, &window_h);

	GLfloat vec[2] = {window_w, window_h};
	glUniform2fv(uniform_window_size, 1, vec);
	vec[0] = textbox->x;
	vec[1] = textbox->y;
	glUniform2fv(uniform_offset, 1, vec);

	glBindBuffer(GL_ARRAY_BUFFER, textbox->vertices_buff);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,4 * sizeof(GLfloat), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}
