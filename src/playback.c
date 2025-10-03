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
#include <stdint.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include <time.h>

#include <song_def.h>
#include <error.h>


#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>


static ALuint source;

static ALuint buffers[2];

static SONG *song_playing = NULL;
static unsigned int song_playing_duration[2] = {0, 0};

static unsigned int playback_position_offset;


void play_single(SONG *current_song) {
  song_playing = current_song;
  char *song_name = malloc(strlen(current_song->filename) + 7);
  sprintf(song_name, "flacs/%s", current_song->filename);

  ALenum error;

  alDeleteSources(1, &source);
  alDeleteBuffers(1, buffers);
  alDeleteBuffers(1, &buffers[1]);

  ALvoid *data = NULL;
  ALsizei size;
  unsigned int frequency, channels;
  drflac_uint64 total_pcm_frames;

  data = (ALvoid *)drflac_open_file_and_read_pcm_frames_s16(song_name, &channels, &frequency, &total_pcm_frames, NULL);

  free(song_name);
  if(data == NULL) {
    trackjack_error(JACK_ERR_DECODER, AL_NO_ERROR);
    return;
  }

  song_playing_duration[0] = total_pcm_frames / 48000;
  song_playing_duration[1] = 0;
  size = total_pcm_frames * sizeof(int16_t) * channels;
  alGenBuffers(1, buffers);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    return;
  }

  ALenum format;


  if(channels == 1) {
    format = AL_FORMAT_MONO16;
  } else {
    format = AL_FORMAT_STEREO16;
  }

  alBufferData(buffers[0], format, data, size, 48000);
  free(data);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    return;
  }

  alGenSources(1, &source);
  alSourceQueueBuffers(source, 1, buffers);
  alSourcePlay(source);

  playback_position_offset = 0;

  return;
}


void play_song(SONG *current_song) {
  song_playing = current_song;
  char *song_name = malloc(strlen(current_song->filename) + 7);
  sprintf(song_name, "flacs/%s", current_song->filename);

  ALenum error;

  if(alIsSource(source) == AL_TRUE) {alDeleteSources(1, &source);}
  alDeleteBuffers(1, buffers);
  alDeleteBuffers(1, &buffers[1]);

  //alutInit(NULL, NULL);

  ALvoid *data = NULL;
  ALsizei size;
  drflac_uint64 total_pcm_frames;

  unsigned int frequency, channels;
  data = (ALvoid *)drflac_open_file_and_read_pcm_frames_s16(song_name, &channels, &frequency, &total_pcm_frames, NULL);

  free(song_name);
  if(data == NULL) {
    trackjack_error(JACK_ERR_DECODER, AL_NO_ERROR);
    return;
  }

  song_playing_duration[0] = total_pcm_frames / 48000;
  size = total_pcm_frames * sizeof(int16_t) * channels;

  error = AL_NO_ERROR;
  alGenBuffers(1, buffers);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    free(data);
    return;
  }

  ALenum format;

  if(channels == 1) {
    format = AL_FORMAT_MONO16;
  } else {
    format = AL_FORMAT_STEREO16;
  }

  alBufferData(buffers[0], format, data, size, 48000);
  free(data);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    return;
  }

  alGenSources(1, &source);
  alSourceQueueBuffers(source, 1, buffers);
  alSourcePlay(source);

  playback_position_offset = 0;

  if(current_song->next == NULL) {return;}
  current_song = current_song->next;

  song_name = malloc(strlen(current_song->filename) + 7);
  sprintf(song_name, "flacs/%s", current_song->filename);




  // Decode next song and load it into queue

  data = (ALvoid *)drflac_open_file_and_read_pcm_frames_s16(song_name, &channels, &frequency, &total_pcm_frames, NULL);

  free(song_name);
  if(data == NULL) {
    trackjack_error(JACK_ERR_DECODER, AL_NO_ERROR);
    return;
  }

  song_playing_duration[1] = total_pcm_frames / 48000;
  size = total_pcm_frames * sizeof(int16_t) * channels;

  alGenBuffers(1, &buffers[1]);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    free(data);
    return;
  }

  if(channels == 1) {
    format = AL_FORMAT_MONO16;
  } else {
    format = AL_FORMAT_STEREO16;
  }

  alBufferData(buffers[1], format, data, size, 48000);
  free(data);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    return;
  }

  alSourceQueueBuffers(source, 1, &buffers[1]);
  return;
}



void playback_checkstate(void) {
  ALint value;
  ALenum error;

  // Check if there is an active source
  if(alIsSource(source) == AL_FALSE) {return;}

  //Check if a song has finished playing
  alGetSourcei(source, AL_BUFFERS_PROCESSED, &value);
  if(value == 0) {return;}

  //Check if another song has been queued
  alGetBufferi(buffers[1], AL_FREQUENCY, &value);
  if((error = alGetError()) != AL_NO_ERROR) {
    alDeleteSources(1, &source);
    alDeleteBuffers(1, buffers);
    song_playing = NULL;
    song_playing_duration[0] = 0;
    song_playing_duration[1] = 0;
    return;
  }

  alSourceUnqueueBuffers(source, 1, buffers);
  alDeleteBuffers(1, buffers);
  buffers[0] = buffers[1];
  song_playing_duration[0] = song_playing_duration[1];

  alGetSourcei(source, AL_SEC_OFFSET, &value);
  playback_position_offset += value;

  // Load next song
  song_playing = song_playing->next;
  SONG *next_song = song_playing->next;
  if(next_song == NULL) {
    buffers[1]++;
    return;
  }
  char *song_name = malloc(strlen(next_song->filename) + 7);

  sprintf(song_name, "flacs/%s", next_song->filename);

  ALvoid *data = NULL;
  ALsizei size;
  unsigned int frequency, channels;
  drflac_uint64 total_pcm_frames;

  data = (ALvoid *)drflac_open_file_and_read_pcm_frames_s16(song_name, &channels, &frequency, &total_pcm_frames, NULL);

  free(song_name);
  if(data == NULL) {
    trackjack_error(JACK_ERR_DECODER, AL_NO_ERROR);
    return;
  }

  song_playing_duration[1] = total_pcm_frames / 48000;
  size = total_pcm_frames * sizeof(int16_t) * channels;

  alGenBuffers(1, &buffers[1]);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    free(data);
    return;
  }

  alBufferData(buffers[1], AL_FORMAT_STEREO16, data, size, 48000);
  free(data);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, error);
    return;
  }

  alSourceQueueBuffers(source, 1, &buffers[1]);

  return;
}


unsigned int retrieve_playback_position(void) {
  if(alIsSource(source) == AL_FALSE) {return 0;}

  ALint sec;
  alGetSourcei(source, AL_SEC_OFFSET, &sec);

  sec -= playback_position_offset;

  return sec;
}


unsigned int retrieve_song_duration(void) {
  if(alIsSource(source) == AL_FALSE) {return 0;}

  return song_playing_duration[0];
}


SONG *retrieve_song_playing(void) {
  return song_playing;
}

int check_playback_active(void) {
  if(alIsSource(source) == AL_TRUE) {return 1;}
  return 0;
}


int check_playback_state(void) {
  ALint state;
  alGetSourcei(source, AL_SOURCE_STATE, &state);
  if(state == AL_PLAYING) {return 0;}
  return 1;
}

void pause_playback(void) {
  alSourcePause(source);
  return;
}

void unpause_playback(void) {
  alSourcePlay(source);
  return;
}



int set_volume(unsigned int val) {
  float gain = val / 100;
  if(val > 150) {return 1;}

  alListenerf(AL_GAIN, gain);

  return 0;
}
