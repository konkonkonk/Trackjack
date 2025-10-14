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



#include <time.h>
#include <unistd.h>


static int last_state;



volatile _Bool stop_thread = 0;

extern int sleep_time;
void playback_update(void);


void usleep_until(int duration) {
  int current_state = clock() * 1000000 / CLOCKS_PER_SEC;
  if(current_state - last_state >= sleep_time) {
    return;
  }

  int diff = current_state - last_state;
  usleep(sleep_time - diff);
}


void *playback_thread(void *) {
  usleep(sleep_time);
  last_state = clock() * 1000000 / CLOCKS_PER_SEC;
  if(stop_thread) {
    stop_thread = 0;
    return NULL;
  }
  playback_update();

  while(!stop_thread) {
    usleep_until(sleep_time);
    last_state = clock() * 1000000 / CLOCKS_PER_SEC;
    playback_update();

  }

  stop_thread = 0;
  return NULL;
}
