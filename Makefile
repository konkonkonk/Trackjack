CC := gcc

SRC := $(wildcard src/*.c)

PROJECTNAME := trackjack

LDFLAGS := -Iheaders -lncurses -lopenal -lalut -lm

OPTPARAM := -O5

DEBUGPARAM :=

compile:
	$(CC) -o $(PROJECTNAME) $(SRC) $(LDFLAGS) $(OPTPARAM) $(DEBUGPARAM)

install: compile
	mv $(PROJECTNAME) /usr/bin/
