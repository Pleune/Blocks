CC=gcc
ROOT=./

CFLAGS:= -Wall -O3 -g --std=c99
LFLAGS:= --std=c99 -g
LIBS:=

ifeq ($(OS),Windows_NT)
	CFLAGS:= $(shell pkg-config --cflags sdl2 SDL2_ttf glew)
	LFLAGS:=
	LIBS:= $(shell pkg-config --libs sdl2 SDL2_ttf glew) -lopengl32 -lm -mconsole
else
#	UNAME_S := $(shell uname -s)
#	ifeq ($(UNAME_S),Linux)
		CFLAGS:= $(shell pkg-config --cflags sdl2 SDL2_ttf gl glew)
		LFLAGS:=
		LIBS:= -lm $(shell pkg-config --libs sdl2 SDL2_ttf gl glew)
#	endif
#	ifeq ($(UNAME_S),Darwin)
#	endif
endif

SRCDIR=$(ROOT)src/
LIBDIR=$(ROOT)lib/
BUILDDIR=$(ROOT)build/
OUTPUTDIR=$(ROOT)bin/

SRC:=$(wildcard $(SRCDIR)*.c)

_OBJS=$(patsubst $(SRCDIR)%.c,%.o,$(SRC))
OBJS=$(patsubst %,$(BUILDDIR)%,$(_OBJS))

NAME=blocks

all:	$(OUTPUTDIR)$(NAME)

-include $(OBJS:.o=.d)

$(BUILDDIR)%.d:	$(SRCDIR)%.c
	gcc $(CFLAGS) -M $< |  sed 's,$*\.o[ :]*,\$(BUILDDIR)$*\.o : ,g' > $@

$(BUILDDIR)%.o:	$(SRCDIR)%.c
	gcc $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)$(NAME): $(OBJS)
	gcc $(LFLAGS) -o $(OUTPUTDIR)$(NAME) $(OBJS) $(LIBS)

check-syntax:
	gcc -s -o /dev/null -S $(CHK_SOURCES)

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)* $(OUTPUTDIR)blocks

