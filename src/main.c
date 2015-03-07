#include <stdio.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "state.h"
#include "state_menu.h"
#include "state_game.h"

enum events {INITALIZE, RUN, CLOSE, MAX_EVENTS};

//FUNCTION DEFS****
static void run();//run the current state's run function
static void transition();//switch to another state

static void cleanup();//shuts everything down for a reboot
static void start();//just turns istransitioning back on
static void initalize();//contains the startup code
static void exitgame();//actually exit

//lookup for what functions do what for what states
void (*const statetable[MAX_STATES][MAX_EVENTS]) (void) = {
	{state_menu_init, state_menu_run, state_menu_close},//menu
	{state_game_init, state_game_run, state_game_close},//game
	{cleanup, start, initalize},//handle starting/closing
	{exitgame, 0, 0}//close the game
};

//run function vs switch states function
static void (*const transitionfunc[2]) (void) = {
	run,
	transition
};

//this is the state started by the STARTING state
const static enum states FIRSTSTATE = GAME;

static int isrunning = 1;
static int istransitioning = 0;

//used by the transition function
static enum states currentstate = STARTING;
static enum states nextstate;

//the main window
SDL_Window *win;
SDL_GLContext glcontext;

int windoww, windowh;

char *basepath;

int
main(int argc, char *argv[])
{
	//do until exit
	while(isrunning)
		//if the program's transition flag is set, run transition()
		//otherwise just run the current state's run function
		(*transitionfunc[istransitioning]) ();
	return 0;
}

//this state engine's "api" call to request a transition to another state
void
state_changeto(enum states newstate)
{
	printf("STATUS: state %i is requesting a change to state %i\n", currentstate, newstate);
	istransitioning = 1;
	nextstate = newstate;
}

static void
//run the current state's run function
run()
{
	(*statetable[currentstate][RUN]) ();
}

//cleanup this state.
//switch to the next state.
//init that state.
//begin running the new state.
static void
transition()
{
	printf("STATUS: transition: \"cleaning up\" last state. cleaning up the STARTING state will initalize the program.\n");
	(*statetable[currentstate][CLOSE]) ();
	printf("STATUS: transition: switching to another state: %i\n", nextstate);
	currentstate = nextstate;
	printf("STATUS: transition: initalizing the new state. if the new state is CLOSING, then this will shutdown the program\n");
	(*statetable[currentstate][INITALIZE]) ();
	istransitioning = 0;
}

//something bad happened, exit
static void
fail(const char *msg)
{
	printf("FAILED: %s\n", msg);
	state_changeto(CLOSING);
}

//the STARTING state functions****

//state: STARTING
//event: INITALIZE
//changing back to the STARTING state will reboot the program.
//INITALIZE will always be called before the close.
//therefore this function is the program's cleanup program.
static void
cleanup()
{
	printf("STATUS: cleaning up for either an exit or a reboot\n");

	free(basepath);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

//state: STARTING
//event: RUN
static void start()
{
	state_changeto(FIRSTSTATE);
}

//state: STARTING
//event: CLOSE
//the close event for the starting state is what gets run before the first main state,
//therefore the programs's init function is this
static void initalize()
{
	printf("STATUS: initalizing the program\n");

	if(SDL_Init(SDL_INIT_EVERYTHING))
		//SDL2 init failed
		fail("SDL_Init(SDL_INIT_EVERYTHING)");

	win = SDL_CreateWindow("Blocks", 100, 100, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(!win)
		//SDL2 didn't create a window
		fail("SDL_CreateWindow");

	glcontext = SDL_GL_CreateContext(win);
	if(!glcontext)
		//SDL2 didint create the required link with opengl
		fail("SDL_GL_CreateContext()");

	if(glewInit())
		//glew couldn't do to wrangling
		fail("glewInit()");

	SDL_GetWindowSize(win, &windoww, &windowh);

	glClearColor(.3,0,0,1);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	basepath = SDL_GetBasePath();
}

//this is the only CLOSING state function****
//state: CLOSING
//event: INITALIZE
//no other functions are in this state because the program exits after this function exits.
static void
exitgame()
{
	//cleanup before closing.
	cleanup();
	printf("STATUS: exiting the program.\n");
	isrunning = 0;
}

//functions used outside this file
void
updatewindowbounds(int w, int h)
{
	glViewport(0, 0, w, h);
	windoww = w;
	windowh = h;
}

void
getwindowsize(int *w, int*h)
{
	*w = windoww;
	*h = windowh;
}

void
swapwindow()
{
	SDL_GL_SwapWindow(win);
}

char *
getbasepath()
{
	return basepath;
}

void
centermouse()
{
	SDL_WarpMouseInWindow(win, windoww/2, windowh/2);
}
