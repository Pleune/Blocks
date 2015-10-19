CC=gcc

ROOT=./

CFLAGS:= -Wall -O1 -g
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

NAME=blocks

all:	$(OUTPUTDIR)$(NAME)

-include $(OBJS:.o=.d)

$(BUILDDIR)%.d:	$(SRCDIR)%.c
	$(CC) -M -I $(INCDIR) $< |  sed 's,$*\.o[ :]*,\$(BUILDDIR)$*\.o : ,g' > $@

$(BUILDDIR)%.o:	$(SRCDIR)%.c
	gcc $(CFLAGS) -I $(INCDIR) -c $(CFLAGS) $< -o $@

$(OUTPUTDIR)$(NAME): $(OBJS)
	gcc $(LFLAGS) -o $(OUTPUTDIR)$(NAME) $(OBJS) $(LIBS)

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)* $(OUTPUTDIR)blocks

