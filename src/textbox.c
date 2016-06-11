#include "textbox.h"

#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

#include "state.h"
#include "debug.h"
#include "gl.h"
#include "stack.h"

#define SYM_ERROR 'E'

struct charcter {
	int w;
	int x;
	unsigned char index;
};

struct font_info_render {
	struct charcter c[255];

	struct {
		GLuint texture;
		int w;
		int h;
	} gl;
};

struct font_info_ttf {
	const char *name;
	const char *path;
	int pt;
};

struct textbox {
	int w;
	int h;
	int x;
	int y;

   	GLuint vertices_buff;
	GLint vertices_num;
	enum font font;
};

static struct font_info_ttf ttf_info[TEXTBOX_NUM_FONTS] = {
	[STANDARD] = {"Standard", "resources/fonts/Roboto/Roboto-Regular.ttf", 16}
};
static struct font_info_render render_info[TEXTBOX_NUM_FONTS];

static GLuint glprogram;
static GLuint uniform_tex;
static GLuint uniform_window_size;

const static char *shader_vertex = "\
#version 300 es\n\
precision mediump float;\n\
layout(location = 0) in vec2 pos;\n\
layout(location = 1) in vec2 Tex;\n\
uniform vec2 window_size;\n\
out vec2 Texcoord;\n\
void main() {\n\
	Texcoord = Tex;\n\
    vec2 p = pos/window_size;\n\
    p.y = 1.0f - p.y;\n\
    p = p * vec2(2,2) - vec2(1,1);\n\
	gl_Position = vec4(p, 0.0, 1.0);\n\
}\
";

const static char *shader_fragment = "\
#version 300 es\n\
precision mediump float;\n\
in vec2 Texcoord;\n\
out vec4 color;\n\
uniform sampler2D tex;\n\
void main()\n\
{\n\
    vec4 texcolor = texture(tex, Texcoord);\n\
//	if(texcolor.a < 0.1)\n\
//        discard;\n\
    color = texcolor;\n\
}\
";

static void
load_font(struct font_info_ttf *ttf_info, struct font_info_render *render_info)
{
	char *file = calloc(1024, sizeof(char));
	strncat(file, state_basepath_get(), 1024);
	strncat(file, ttf_info->path, 1024 - strlen(file));

	TTF_Font *font = TTF_OpenFont(file, ttf_info->pt);

	if(!font)
		fail("Unable to load font: %s", file);

	const static char charcters[] = "a b b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z 1 2 3 4 5 6 7 8 9 0 ! @ # $ % ^ & * ( ) - _ = + [ { ] } \\ | , . < > / ? ; : ' \" ` ~  ";
	const static SDL_Color background = { 255, 255, 255, 255};

	TTF_SetFontKerning(font, 0);

	SDL_Surface *surface = TTF_RenderText_Blended(font, charcters, background);

	info("BitsPerPixel %i", surface->format->BitsPerPixel);
	info("Rmask %08x", surface->format->Rmask);
	info("Gmask %08x", surface->format->Gmask);
	info("Bmask %08x", surface->format->Bmask);
	info("Amask %08x", surface->format->Amask);

	memset(render_info, 0, sizeof(struct font_info_render));

	glGenTextures(1, &(render_info->gl.texture));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, render_info->gl.texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	size_t size = surface->w * surface->h;

	size_t i;
	for(i=0; i<size; ++i)
	{
		uint32_t color = ((uint32_t *)(surface->pixels))[i];
		uint32_t tmp = (color << 8) | (color >> 24);

		((uint32_t *)(surface->pixels))[i] = tmp;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h,
				 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, surface->pixels);

	render_info->gl.h = surface->h;
	render_info->gl.w = surface->w;

	int width_build = 1;
	for(i=0; i<sizeof(charcters); i++)
	{
		struct charcter *c = &render_info->c[(unsigned char) charcters[i]];

		TTF_GlyphMetrics(font, charcters[i], 0,0,0,0, &c->w);
		c->x = width_build;
		width_build += c->w;
	}

	SDL_FreeSurface(surface);
	TTF_CloseFont(font);
}

static void
unload_font(struct font_info_render *render_info)
{
   	glDeleteTextures(1, &render_info->gl.texture);
}

void
textbox_static_init()
{
	gl_program_load_str(&glprogram, shader_vertex, shader_fragment);
	uniform_tex = glGetUniformLocation(glprogram, "tex");
	uniform_window_size = glGetUniformLocation(glprogram, "window_size");

	int i;
	for(i=0; i<TEXTBOX_NUM_FONTS; i++)
		load_font(&ttf_info[i], &render_info[i]);
}

void
textbox_static_cleanup()
{
	int i;
	for(i=0; i<TEXTBOX_NUM_FONTS; i++)
		unload_font(&render_info[i]);
}

textbox_t *
textbox_create(int x, int y, int w, int h, const char* txt, enum font font)
{
	textbox_t *textbox = malloc(sizeof(textbox_t));

	textbox->x = x;
	textbox->y = y;
	textbox->w = w;
	textbox->h = h;

	textbox->font = font;

	glGenBuffers(1, &textbox->vertices_buff);

	textbox_set_txt(textbox, txt);

	return textbox;
}

void
textbox_destroy(struct textbox* textbox)
{
	glDeleteBuffers(1, &textbox->vertices_buff);
	free(textbox);
}

static inline void
add_point(struct stack *buffer, GLfloat x, GLfloat y, GLfloat u, GLfloat v)
{
	stack_push(buffer, &x);
	stack_push(buffer, &y);
	stack_push(buffer, &u);
	stack_push(buffer, &v);
}

static inline void
add_char(struct stack *buffer, GLfloat x, GLfloat y, GLfloat w, GLfloat h, struct font_info_render *render_info, unsigned char c)
{
	GLfloat char_x = render_info->c[c].x / (GLfloat)render_info->gl.w;
	GLfloat char_w = render_info->c[c].w / (GLfloat)render_info->gl.w;

	add_point(buffer, x, y, char_x, 0);
	add_point(buffer, x, y+h, char_x, 1);
	add_point(buffer, x+w, y, char_x + char_w, 0);

	add_point(buffer, x+w, y+h, char_x + char_w, 1);
	add_point(buffer, x+w, y, char_x + char_w, 0);
	add_point(buffer, x, y+h, char_x, 1);
}

void
textbox_set_txt(textbox_t *textbox, const char *txt)
{
	struct stack points;
	stack_init(&points, sizeof(GLfloat), 100, 5);

	int entire_h = render_info[textbox->font].gl.h;

	int num_points = 0;
	int x = textbox->x;
	int y = textbox->y;
	size_t i;
	for(i=0; txt[i] != 0; i++)
	{
		int char_w = render_info[textbox->font].c[(unsigned char) txt[i]].w;
		int char_h = entire_h;

		add_char(&points, x, y, char_w, char_h, &render_info[textbox->font], txt[i]);
		num_points += 6;
		x += char_w;
	}

	textbox->vertices_num = num_points;

	glBindBuffer(GL_ARRAY_BUFFER, textbox->vertices_buff);
	glBufferData(GL_ARRAY_BUFFER, 4 * num_points * sizeof(GLfloat), points.data, GL_STATIC_DRAW);

	stack_destroy(&points);
}

void
textbox_render(textbox_t* textbox)
{
	glDisable(GL_DEPTH_TEST);
	glUseProgram(glprogram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, render_info[textbox->font].gl.texture);
	glUniform1i(uniform_tex, 0);

	int window_w, window_h;
	state_window_get_size(&window_w, &window_h);

	GLfloat window_vec[2] = {window_w, window_h};

	glUniform2fv(uniform_window_size, 1, window_vec);

	glBindBuffer(GL_ARRAY_BUFFER, textbox->vertices_buff);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,4 * sizeof(GLfloat), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, textbox->vertices_num);
}
