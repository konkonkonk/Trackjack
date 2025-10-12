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
#include <string.h>
#include <AL/al.h>
#include <libavutil/error.h>
#include <ncurses.h>
#include <errno.h>

#include <error_codes.h>
#include <ui.h>


typedef union lib_error {
  ALenum al_error;
  int av_error;
  int generic;
} LIB_ERROR;


void trackjack_error(int error, LIB_ERROR liberr) {
  char *final_msg;

  switch(error) {
    case JACK_ERR_DECODER:
      display_msg("TJ_ERR: Failed to load file.");
      move(0, 0);
      break;
    case JACK_ERR_BUFFERGEN:
      char msg[] = "TJ_ERR: Failed to create or fill audio buffer --- openAL message: ";
      const char *al_msg = alGetString(liberr.al_error);
      final_msg = malloc(strlen(msg) + strlen(al_msg));

      sprintf(final_msg, "%s%s", msg, al_msg);
      display_msg(final_msg);
      free(final_msg);
      move(0, 0);
      break;
    case JACK_ERR_LIBAV_MSG:
      char msg2[] = "FFMPEG: ";
      const char *av_msg = av_err2str(liberr.av_error);
      final_msg = malloc(strlen(msg2) + strlen(av_msg));

      sprintf(final_msg, "%s%s", msg2, av_msg);
      display_msg(final_msg);
      free(final_msg);
      move(0, 0);
      break;
    case JACK_ERR_OPENDIR:
      display_msg("TJ_ERR: Failed to open directory. --- errno: ");
      display_msg(strerror(liberr.generic));

      break;
    case JACK_ERR_PLAYBACK_SOURCE_PREP:
      display_msg("TJ_ERR: Failed to prep audio source for playback.");
      break;
  }

  return;
}
