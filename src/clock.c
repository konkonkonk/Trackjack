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



#include <unistd.h>
#include <time.h>

#define FRAMETIME_MS 33

static int last_frame_clockstate;

void init_clock(void) {
  last_frame_clockstate = clock() * 1000 / CLOCKS_PER_SEC;
  return;
}

void sleep_until_next_tick(void) {
  int current_state = clock() * 1000 / CLOCKS_PER_SEC;
  if(current_state - last_frame_clockstate >= FRAMETIME_MS) {
    last_frame_clockstate = current_state;
    return;
  }
  int difference = current_state - last_frame_clockstate;

  usleep((FRAMETIME_MS - difference) * 1000);
  last_frame_clockstate = current_state;

  return;
}
