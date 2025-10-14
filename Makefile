CC := gcc

SRC := $(wildcard src/*.c)

PROJECTNAME := trackjack

LDFLAGS := -Iheaders -lncurses -lopenal -lalut -lm -lavcodec -lavformat -lavutil -lswresample

OPTPARAM := -O0

DEBUGPARAM := -g

compile:
	$(CC) -o $(PROJECTNAME) $(SRC) $(LDFLAGS) $(OPTPARAM) $(DEBUGPARAM)

install: compile
	mv $(PROJECTNAME) /usr/bin/
