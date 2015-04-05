CC=gcc

ROOT=./

CFLAGS:= -Wall -O2 -g
LFLAGS:= -g
LIBS:=-lm -lSDL2 -lGL -lGLEW

SRCDIR=$(ROOT)src/
INCDIR=$(ROOT)include/
LIBDIR=$(ROOT)lib/
BUILDDIR=$(ROOT)build/
OUTPUTDIR=$(ROOT)bin/

SRC:=$(wildcard $(SRCDIR)*.c)

_OBJS=$(patsubst $(SRCDIR)%.c,%.o,$(SRC))
OBJS=$(patsubst %,$(BUILDDIR)%,$(_OBJS))

NAME=run


%.o:	$(SRCDIR)%.c 
	gcc $(CFLAGS) -I $(INCDIR) -c $(CFLAGS) $< -o $(BUILDDIR)$@

blocks: $(_OBJS)
	gcc $(LFLAGS) -o $(OUTPUTDIR)blocks $(OBJS) $(LIBS)

all:	blocks

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)*.o $(OUTPUTDIR)blocks

