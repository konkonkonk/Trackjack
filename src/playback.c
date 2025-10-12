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
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/dict.h>

#include <AL/al.h>
#include <AL/alext.h>

#include <error_codes.h>
#include <error.h>
#include <ui.h>



#define BASE_TEN 10


typedef struct {
  char *track;
  char *album;
  char *artist;
  char *features;
  int year;
  int duration;
} METADATA;

typedef struct {
  AVFormatContext *format_context;
  AVCodecParameters *codec_param;
  const AVCodec *codec;
  AVCodecContext *codec_context;
  SwrContext *swr_context;
  METADATA *metadata;
  unsigned int channels;
  unsigned int samplerate;
} AUDIO_SOURCE;

#define META_TRACK_TITLE 0
#define META_ALBUM_TITLE 1
#define META_ALBUM_ARTIST 2
#define META_TRACK_ARTISTS 3
#define MAX_META_TYPE_STR 3

#define META_TRACK_DURATION 4
#define META_YEAR 5
#define MAX_META_TYPE 5


#define MAX_VOLUME 180


static AUDIO_SOURCE *active_sources[2] = {NULL, NULL};

static AVPacket *pPacket = NULL;
static AVFrame *pFrame = NULL;


static ALuint source;
static ALuint buffers[2];


int sleep_time = 10000;


static pthread_t thread;

void *playback_thread(void *);

void playback_init(void) {
  // Tell ffmpeg to shut up
  av_log_set_level(AV_LOG_QUIET);
  pPacket = av_packet_alloc();
  pFrame = av_frame_alloc();

  pthread_create(&thread, NULL, playback_thread, NULL);

  return;
}


char *metadata_retrieve_str(int meta_type) {
  if(active_sources[0] == NULL) {return NULL;}
  if(meta_type > MAX_META_TYPE_STR) {return NULL;}

  switch(meta_type) {
    case META_TRACK_TITLE:
      return active_sources[0]->metadata->track;
    case META_ALBUM_TITLE:
      return active_sources[0]->metadata->album;
    case META_ALBUM_ARTIST:
      return active_sources[0]->metadata->artist;
    case META_TRACK_ARTISTS:
      return active_sources[0]->metadata->features;
  }

  return NULL;
}


int metadata_retrieve_int(int meta_type) {
  if(active_sources[0] == NULL) {return 0;}
  if(meta_type <= MAX_META_TYPE_STR || meta_type > MAX_META_TYPE) {return 0;}

  switch(meta_type) {
    case META_TRACK_DURATION:
      return active_sources[0]->metadata->duration;
    case META_YEAR:
      return active_sources[0]->metadata->year;
  }

  return 0;
}


void kill_openal_source(void) {
  alDeleteSources(1, &source);
  alDeleteBuffers(1, buffers);
  alDeleteBuffers(1, &buffers[1]);

  // Any errors in the above function calls should be ignored
  // The openal error buffer is cleared here to prevent it accidentally being read later
  alGetError();
  return;
}


int playback_read_clock(void) {
  ALint val;
  if(alIsSource(source) != AL_TRUE) {return 0;}
  alGetSourcei(source, AL_SEC_OFFSET, &val);

  return val;
}





void bind_chunk(AUDIO_SOURCE *song, uint8_t *data, int size, ALuint buffer) {
  ALenum format, error;
  if(song->channels == 1) {
    format = AL_FORMAT_MONO_FLOAT32;
  }
  else {
    format = AL_FORMAT_STEREO_FLOAT32;
  }

  alBufferData(buffer, format, data, size, song->samplerate);
  free(data);

  if((error = alGetError()) != AL_NO_ERROR) {
    trackjack_error(JACK_ERR_BUFFERGEN, (LIB_ERROR)error);
  }

  return;
}


uint8_t *decode_chunk(AUDIO_SOURCE *song, int *buf_size) {
  uint8_t **buf = NULL;

  int channels = song->channels;


  int dst_nb_samples;
  int dst_linesize;
  int err = 0;
  _Bool frame_read = 0;

  while(av_read_frame(song->format_context, pPacket) >= 0) {
    if((err = avcodec_send_packet(song->codec_context, pPacket)) < 0) {
      continue;
    }

    err = avcodec_receive_frame(song->codec_context, pFrame);
    if(err == AVERROR(EAGAIN)) {
      continue;
    }

    frame_read = 1;
    break;
  }

  if(frame_read == 0) {return NULL;}

  dst_nb_samples = swr_get_out_samples(song->swr_context, pFrame->nb_samples);
  av_samples_alloc_array_and_samples(&buf, &dst_linesize, channels, dst_nb_samples, AV_SAMPLE_FMT_FLT, 0);

  swr_convert(song->swr_context, buf, dst_nb_samples, (const uint8_t **)pFrame->extended_data, pFrame->nb_samples);
  


  *buf_size = dst_linesize;
  uint8_t *ret = malloc(dst_linesize);
  int i;
  for(i = 0; i < dst_linesize; i++) {
    ret[i] = buf[0][i];
  }

  av_packet_unref(pPacket);

  if(buf) {
    av_freep(&buf[0]);
  }
  av_freep(&buf);


  return ret;
}




void free_audio_source(AUDIO_SOURCE *song) {
  if(song->codec_context) {avcodec_free_context(&song->codec_context);}
  if(song->swr_context) {swr_free(&song->swr_context);}
  if(song->format_context) {avformat_close_input(&song->format_context);}

  METADATA *meta = song->metadata;
  if(meta->artist) {free(meta->artist);}
  if(meta->album) {free(meta->album);}
  if(meta->track) {free(meta->track);}
  if(meta->features) {free(meta->features);}
  free(meta);

  free(song);
}


void playback_cleanup(void) {
  kill_openal_source();

  if(active_sources[0]) {free_audio_source(active_sources[0]);}
  if(active_sources[1]) {free_audio_source(active_sources[1]);}

  if(pPacket) {av_packet_free(&pPacket);}
  if(pFrame) {av_frame_free(&pFrame);}

  pthread_cancel(thread);
  pthread_join(thread, NULL);

  return;
}



AUDIO_SOURCE *new_audio_source(const char *filename) {
  AUDIO_SOURCE  *new_song = calloc(1, sizeof(AUDIO_SOURCE));

  new_song->format_context = avformat_alloc_context();
  int ret = avformat_open_input(&new_song->format_context, filename, NULL, NULL);
  if(ret < 0) {
    trackjack_error(JACK_ERR_LIBAV_MSG, (LIB_ERROR)ret);
    return NULL;
  }

  ret = avformat_find_stream_info(new_song->format_context, NULL);
  if(ret < 0) {
    trackjack_error(JACK_ERR_LIBAV_MSG, (LIB_ERROR)ret);
    return NULL;
  }


  new_song->codec_param = new_song->format_context->streams[0]->codecpar;
  if(new_song->codec_param->codec_type != AVMEDIA_TYPE_AUDIO) {
    return NULL;
  }
  METADATA *meta = calloc(1, sizeof(METADATA));

  new_song->channels = new_song->codec_param->ch_layout.nb_channels;
  new_song->samplerate = new_song->codec_param->sample_rate;
  meta->duration = new_song->format_context->duration / AV_TIME_BASE;


  const AVDictionaryEntry *tag = NULL;
  char *garbage_ptr; //strtol() wants a double pointer to direct us to the remaining part of the date string. it is not needed in this case, but must be created anyway
  while((tag = av_dict_iterate(new_song->format_context->metadata, tag))) {
    if(strcmp(tag->key, "ALBUM") == 0) {meta->album = strdup(tag->value);}
    if(strcmp(tag->key, "TITLE") == 0) {meta->track = strdup(tag->value);}
    if(strcmp(tag->key, "album_artist") == 0) {meta->artist = strdup(tag->value);}
    if(strcmp(tag->key, "ARTIST") == 0) {meta->features = strdup(tag->value);}
    if(strcmp(tag->key, "DATE") == 0) {meta->year = strtol(tag->value, &garbage_ptr, BASE_TEN);}
  }
  new_song->metadata = meta;

  return new_song;
}


int prep_audio_source(AUDIO_SOURCE *new) {
  new->codec = avcodec_find_decoder(new->codec_param->codec_id);
  new->codec_context = avcodec_alloc_context3(new->codec);

  int ret = avcodec_parameters_to_context(new->codec_context, new->codec_param);
  if(ret < 0) {
    trackjack_error(JACK_ERR_LIBAV_MSG, (LIB_ERROR)ret);
    return -1;
  }

  ret = avcodec_open2(new->codec_context, new->codec, NULL);
  if(ret < 0) {
    trackjack_error(JACK_ERR_LIBAV_MSG, (LIB_ERROR)ret);
    return -2;
  }

  // Initialize swresample to convert sample format later
  AVChannelLayout *src_ch_layout = &new->codec_context->ch_layout;
  AVChannelLayout *dst_ch_layout = src_ch_layout;

  unsigned int src_sample_fmt = new->codec_context->sample_fmt;
  unsigned int dst_sample_fmt = AV_SAMPLE_FMT_FLT;

  int src_rate = new->samplerate;
  int dst_rate = src_rate;

  ret = swr_alloc_set_opts2(&new->swr_context, dst_ch_layout, dst_sample_fmt, dst_rate, src_ch_layout, src_sample_fmt, src_rate, 0, NULL);
  if(ret < 0) {
    trackjack_error(JACK_ERR_LIBAV_MSG, (LIB_ERROR)ret);
    return -3;
  }

  ret = swr_init(new->swr_context);
  if(ret < 0) {
    trackjack_error(JACK_ERR_LIBAV_MSG, (LIB_ERROR)ret);
    return -4;
  }

  return 0;
}



void playback_update(void) {

  if(active_sources[0] == NULL) {return;}

  ALint value;
  alGetSourcei(source, AL_BUFFERS_PROCESSED, &value);
  if(value == 0) {return;}
  buffers[0] = buffers[1];
  buffers[1]++;

  uint8_t *buf = NULL;
  int buf_size;
  if((buf = decode_chunk(active_sources[0], &buf_size)))
    {
    alGenBuffers(1, &buffers[1]);
    bind_chunk(active_sources[0], buf, buf_size, buffers[1]);
    alSourceQueueBuffers(source, 1, &buffers[1]);
  }
  else if(active_sources[1])
    {

    prep_audio_source(active_sources[1]);
    alGenBuffers(1, &buffers[1]);
    bind_chunk(active_sources[1], buf, buf_size, buffers[1]);
    alSourceQueueBuffers(source, 1, &buffers[1]);

    free_audio_source(active_sources[0]);
    active_sources[0] = active_sources[1];
    active_sources[1] = NULL;
  }
  else
    {
    active_sources[0] = NULL;
  }

  return;
}




void playback_start(const char *filename) {
  AUDIO_SOURCE *new_song = new_audio_source(filename);

  if(new_song == NULL) {return;}

  int ret = prep_audio_source(new_song);
  if(ret < 0) {
    trackjack_error(JACK_ERR_PLAYBACK_SOURCE_PREP, (LIB_ERROR)ret);
    free_audio_source(new_song);
    return;
  }

  switch(new_song->samplerate) {
    case 44100:
      sleep_time = 60000;
      break;
    case 48000:
      sleep_time = 50000;
      break;
    case 96000:
      sleep_time = 30000;
      break;
    case 192000:
      sleep_time = 10000;
      break;
    sleep_time = 15000;
  }

  // Two chunks are loaded instead of one
  // because while the second chunk is playing later,
  // the third will be loaded. And so on
  int buf_size;
  int buf_size_2;
  uint8_t *buf = decode_chunk(new_song, &buf_size);
  uint8_t *buf_2 = decode_chunk(new_song, &buf_size_2);

  //kill thread
  pthread_cancel(thread);
  pthread_join(thread, NULL);

  kill_openal_source();
  alGenSources(1, &source);

  alGenBuffers(2, buffers);

  bind_chunk(new_song, buf, buf_size, buffers[0]);
  bind_chunk(new_song, buf_2, buf_size_2, buffers[1]);
  alSourceQueueBuffers(source, 2, buffers);
  alSourcePlay(source);


  if(active_sources[0]) {free_audio_source(active_sources[0]);}
  if(active_sources[1]) {free_audio_source(active_sources[1]);}

  active_sources[0] = new_song;
  active_sources[1] = NULL;

  display_metadata_bar(new_song->metadata->album, new_song->metadata->artist, new_song->metadata->year, new_song->metadata->features);
  display_song_playback_bar(new_song->metadata->track);


  pthread_create(&thread, NULL, playback_thread, NULL);

  return;
}


void playback_queue(const char *filename) {
  AUDIO_SOURCE *new_song = new_audio_source(filename);
  if(new_song == NULL) {return;}

  if(active_sources[1]) {free_audio_source(active_sources[1]);}
  active_sources[1] = new_song;

  return;
}




int set_master_volume(unsigned int val) {
  float gain = val / 100;
  if(val > MAX_VOLUME) {return 1;}

  ALenum error;
  alListenerf(AL_GAIN, gain);
  if((error = alGetError()) != AL_NO_ERROR) {
    display_msg((char *)alGetString(error));
    return 0;
  }

  return 0;
}



int check_playback_active(void) {
  if(active_sources[0]) {return 0;}
  return 1;
}

int check_playback_state(void) {
  ALint state;
  alGetSourcei(source, AL_SOURCE_STATE, &state);
  if(state == AL_PLAYING) {return 0;}
  return 1;
}


void playback_pause(void) {
  alSourcePause(source);

  // Clear error buffer to avoid misreading it later
  alGetError();
  return;
}

void playback_unpause(void) {
  alSourcePlay(source);

  // ibid.
  alGetError();
  return;
}
