/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *-----------------------------------------------------------------
 * Sensory TrulyHandsfree SDK: audio.h
 * Provides support for asynchronous audio input.
 *-----------------------------------------------------------------
 */

#ifndef _AUDIO_H
#define _AUDIO_H

#if defined(__cplusplus) 
extern "C" {
#endif

#define AUDIO_BUFFERSZ 250
#define SAMPLERATE 16000
#define NUMCHANNELS 1

#if defined(__linux__)
# define USE_ALSA
//# define USE_OSS
#else
# define AUDIO_BUFFERS 30
#endif


typedef struct audiol_s {
  short *audio;
  struct audiol_s *n;
  unsigned short len;
  unsigned flags;
} audiol_t;

#if !defined(_WINDOWS) && !defined(WIN32)
# ifndef HINSTANCE
#  define HINSTANCE void *
# endif
#else
#include <windows.h>
#endif

/* Audio */
int startRecord(void);
int stopRecord(void);
int initAudio(HINSTANCE inst, void *c);
int initAudio_ex(HINSTANCE inst, void *c, unsigned short *file_input);
void killAudio(void);
void audioEventLoop(void);
audiol_t *audioGetNextBuffer();
void audioNukeBuffer(audiol_t *p);
#if defined(_WINDOWS) || defined(_WIN32_) || defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
  int audioPanic(char *why, MMRESULT res);
#else
int audioPanic(char *why);
#endif
#ifdef USE_ALSA
  int audioPlay(short *buffer, int len);
#endif
#if defined(__cplusplus) 
}
#endif
#endif



