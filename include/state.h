#ifndef STATE_H
#define STATE_H

enum states {MENU, GAME, STARTING, CLOSING, MAX_STATES};

void state_changeto(enum states newstate);

void swapWindow();
char *getbasepath();

#endif
