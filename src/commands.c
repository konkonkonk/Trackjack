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

#include <song_def.h>
#include <playlist.h>
#include <screen.h>
#include <playback.h>

extern SONG *current_head;
extern int current_len;
extern int y;

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
    display_msg("vol - Set master volume");
    display_msg("lp - Load playlist");
    display_msg("lt - Load track");
    display_msg("llp - Load studio album");
    display_msg("lhdir - Load new home folder");
    display_msg("atp - Add to playlist");
    display_msg("np - New playlist");
    display_msg("lspl - List playlists");
    display_msg("lslp - List studio albums");
    display_msg("lsep - List EPS");
    display_msg("lssi - List singles");
  }



  else if(strcmp(command, "vol") == 0) {
    // SET VOLUME

    mvprintw(34, 1, ": Enter volume percentage: ");
    move(34, 29);
    getstr(buffer);
    unsigned int volume = atoi(buffer);

    if(set_volume(volume) == 0) {display_msg("Succesfully changed volume.");
    } else {display_msg("Selected value exceeds 150. Volume unchanged.");}

  }


  else if(strcmp(command, "lp") == 0) {
    // LOAD PLAYLIST FROM HOME DIRECTORY

    mvprintw(34, 1, ": Playlist name:       ");
    move(34, 18);
    getstr(buffer);
    PLAYLIST *new_list = read_playlist(buffer);
    clear();
    display_playlist(new_list);

    char msg[] = "Loaded playlist: ";
    final_msg = malloc(strlen(msg) + strlen(new_list->name));
    sprintf(final_msg, "%s%s", msg, new_list->name);
    display_msg(final_msg);
    free(final_msg);

    current_head = new_list->head;
    current_len = new_list->len;
    free(new_list);
  }

  else if(strcmp(command, "lt") == 0) {
    // LOAD TRACK FROM HOME DIRECTORY

    mvprintw(34, 1, ": Track name:        ");
    move(34, 15);
    refresh();
    getstr(buffer);
    SONG *new_track = new_song(buffer);
    play_single(new_track);

    char msg[] = "Loaded track: ";
    final_msg = malloc(strlen(msg) + strlen(new_track->filename));
    sprintf(final_msg, "%s%s", msg, new_track->filename);
    display_msg(final_msg);
    free(final_msg);

  }

  else if(strcmp(command, "lhdir") == 0) {
    // LOAD NEW HOME DIRECTORY

    mvprintw(34, 1, ": New home directory: ");
    move(34, 23);
    refresh();
    getstr(buffer);
    chdir(buffer);

}

  else if(strcmp(command, "atp") == 0) {
    // ADD TO PLAYLIST

    mvprintw(34, 1, ": Playlist to change: ");
    move(34, 23);
    refresh();
    getstr(buffer);
    PLAYLIST *temp = read_playlist(buffer);

    mvprintw(34, 1, ": Song to add:                       ");
    move(34, 16);
    refresh();
    getstr(buffer);
    mvprintw(34, 1, "                                        ");
    SONG *temp_s = new_song(buffer);
    add_song(temp->head, temp_s);
    temp->len = get_list_len(temp->head);

    write_playlist(temp);
  }

  else if(strcmp(command, "np") == 0) {
    // CREATE NEW PLAYLIST

    mvprintw(34, 1, ": Name of new playlist: ");
    move(34, 26);
    refresh();
    getstr(buffer);

    mvprintw(34, 1, ": Name of the first song:                  ");
    move(34, 28);
    refresh();
    getstr(bufferB);
    SONG *temp_s_head = new_song(bufferB);
    mvprintw(34, 1, "                                           ");

    write_playlist(generate_playlist(temp_s_head, buffer));
  }

  else if(strcmp(command, "llp") == 0) {
    // LOAD STUDIO ALBUM

    mvprintw(34, 1, ": Name of LP: ");
    move(34, 15);
    refresh();
    getstr(buffer);
    clear();

    COM_RELEASE *new_lp = read_com_release(buffer, 0);
    display_com_release(new_lp);

    char msg[] = "Loaded studio album: ";
    final_msg = malloc(strlen(msg) + strlen(new_lp->title));
    sprintf(final_msg, "%s%s", msg, new_lp->title);
    display_msg(final_msg);
    free(final_msg);

    current_head = new_lp->track_head;
    current_len = new_lp->track_count;
    free_com_release(new_lp);
  }

  else if(strcmp(command, "lspl") == 0) {
    // LIST PLAYLISTS

    DIR *d;
    struct dirent *dir;
    d = opendir("playlists/");

    move(34, 1);
    if(d) {
      while((dir = readdir(d)) != NULL) {
        printw("%s ", dir->d_name);
      }
      closedir(d);
    }

  }

  else if(strcmp(command, "lslp") == 0) {
    // LIST STUDIO ALBUMS

    DIR *d;
    struct dirent *dir;
    d = opendir("LP/");

    //move(34, 1);
    display_msg("List of studio albums in current trackjack home folder:");
    if(d) {
      while((dir = readdir(d)) != NULL) {
        //printw("%s ", dir->d_name);
        if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {display_msg(dir->d_name);}
      }
      closedir(d);
    }
  }

  else if(strcmp(command, "lsep") == 0) {
    // LIST EPS

    DIR *d;
    struct dirent *dir;
    d = opendir("EP/");

    move(34, 1);
    if(d) {
      while((dir = readdir(d)) != NULL) {
        printw("%s ", dir->d_name);
      }
      closedir(d);
    }
  }

  else if(strcmp(command, "lssi") == 0) {
    // LIST SINGLES

    DIR *d;
    struct dirent *dir;
    d = opendir("Single/");

    move(34, 1);
    if(d) {
      while((dir = readdir(d)) != NULL) {
        printw("%s ", dir->d_name);
      }
      closedir(d);
    }
  }

  mvprintw(34, 1, "                                                   ");

  free(buffer);
  free(bufferB);

  return;
}
