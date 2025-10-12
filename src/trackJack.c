/*
Copyright (C) 2025 Quinn Borrok

This file is part of Trackjack.

Trackjack is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Trackjack is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see <https://www.gnu.org/licenses/>.

*/




#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <AL/al.h>
#include <AL/alut.h>

#include <playback.h>
#include <clock.h>
#include <ui.h>

#define KEY_ESC 28
#define KEY_CR 10
#define KEY_SPC 32
#define KEY_COLON 58


#define NO_COMMAND_HINT NULL


_Bool paused = 0;

void parse_cmd(char *command);


void init(void) {

  initscr();
  curs_set(TRUE);
  keypad(stdscr, TRUE);
  init_ui();
  init_clock();
  playback_init();
  alutInit(NULL, NULL);

}




int main(int argc, char **argv) {
  char *command = malloc(160);
  _Bool first_loop = 1;

  if(argc > 3) {
    fprintf(stderr, "Too many arguments.\nUse --help for help.\n");
    return -1;
  }

  if(argc > 1) {
    if(strcmp(argv[1], "--help") != 0 && strcmp(argv[1], "--script") != 0) {
      fprintf(stderr, "Unrecognized argument '%s'\n", argv[1]);
      return -2;
    }

    if(strcmp(argv[1], "--help") == 0) {
      printf("Trackjack plays music.\n--help to print this screen.\n--script to run a script.\n");
      return 0;
    }


  }

  init();
  chdir("test_homedir/");
  ui_open_dir(".");

  display_msg("Welcome to Trackjack. Type \':\' to open command window. Use command \'h\' for help.");

  int ch;
  char *name = NULL;
  _Bool type;
  _Bool exit = 0;
  int fs_index = 0;
  int last_pos = 0;

  noecho();
  nodelay(stdscr, 1);
  while(ch != KEY_ESC && exit == 0) {
    ch = getch();

    switch(ch) {
      case KEY_UP:
        user_nav_up();
        break;
      case KEY_DOWN:
        user_nav_down();
        break;
      case KEY_CR:
        name = retrieve_fs_element(&type, &fs_index);
        if(type == 0) {
          ui_open_dir(name);
        }
        else {
          playback_start(name);
        }
        free(name);
        break;
      case KEY_SPC:
        if(check_playback_state()) {
          playback_unpause();
        }
        else {
          playback_pause();
        }
        break;
      case KEY_COLON:
        curs_set(TRUE);
        display_command_bar(NO_COMMAND_HINT);
        nodelay(stdscr, 0);
        echo();
        refresh();
        getstr(command);

        if(strcmp(command, "q") == 0) {
          exit = 1;
        }
        else {
          parse_cmd(command);
        }

        noecho();
        clear_command_bar();
        nodelay(stdscr, 1);
        break;
    }

    update_msgbox();

    if(last_pos != playback_read_clock()) {
      last_pos = playback_read_clock();
      display_playback_bar();
    }

    reset_cursor();


    refresh();
    sleep_until_next_tick();
  }
  nodelay(stdscr, 0);

  free(command);

  cleanup_ui();
  playback_cleanup();
  alutExit();
  endwin();
  return 0;
}
