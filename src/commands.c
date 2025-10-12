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
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ncurses.h>

#include <ui.h>
#include <playback.h>


void parse_cmd(char *command) {
  char *buffer = malloc(160);
  char *bufferB = malloc(160);
  char *final_msg;
  int i;


  if(strcmp(command, "h") == 0) {
    display_msg("Use return key to start audio playback, and the up and down arrows to navigate. The space bar can be used to pause and unpause the active track.");
    display_msg("All other functionality is available via command line.");
    display_msg("For a list of commands available, use the \'lscmd\' command.");
  }

  else if(strcmp(command, "lscmd") == 0) {
    display_msg("Command list:");
    display_msg("vol - Set master volume (0-180)");
    display_msg("I removed most of the commands because they sucked. New ones will follow.");
  }



  else if(strcmp(command, "vol") == 0) {
    // SET VOLUME

    display_command_bar("Enter volume percentage: ");
    getstr(buffer);
    unsigned int volume = atoi(buffer);

    if(set_master_volume(volume) == 0) {display_msg("Succesfully changed volume.");
    } else {display_msg("Selected value exceeds 180. Volume unchanged.");}

  }




  free(buffer);
  free(bufferB);

  return;
}
