/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: tts.c
 *---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <datalocations.h>

#define STATICDATA 0     // Use static data or read voice database from file
#define FULLSYNTHESIS  1 // Full or Pipelined synthesis

#define LANG       "US English Caller ID"
#define PHRASE1    "Dr John Smith"
#define PHRASE2    "Michael Rogers" 
#define PHRASE3    "Call from, Thomas, Jefferson"
#define PITCH (1.0f)
#define RATE  (1.0f)
#define DURATION (1.0f)
#define SAMPLERATE  16000
#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")

#ifdef _WIN32
# define snprintf _snprintf
#endif

// Prosody options:
// bits 7:0   0 for normal prosody
//            1 for caller ID names prosody
// bit 8      reduces pitch range by 1/2 during synthesis
// bit 9      synthesizes utterances in pieces based on phrase boundaries
//            (ie, commas, periods etc)
//            this restarts the prosody for each phrase. only supported if
//            FULLSYNTHESIS is selected.
// bit 10     resamples from 16KHz to 8KHz.  only supported if
//            FULLSYNTHESIS is selected.


#define PROSODY (0x201)  // names prosody, restart each phrase
#define VOCODER (1)   // default vocoder - only supported value atm

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }

#define CC(a) if(!a) THROW("sfs");

/* Display TTS parameters
 */
static void displaySettings(thf_t *ses, tts_t *tts, void *cons)
{
#define MAXSTR 511
  char str[MAXSTR+1];
  const char *ewhere, *ewhat=NULL;
  int value;
  
  if(!thfTtsConfigGet(ses, tts, PROSODY_DOMAIN, &value))
    THROW("thfTtsConfigGet");
  snprintf(str, MAXSTR, "PROSODY_DOMAIN: %s",
	   value==GENERIC?"generic":"names");
  disp(cons,str);
  if(!thfTtsConfigGet(ses, tts, PROSODY_PITCHRANGE, &value))
    THROW("thfTtsConfigGet");
  snprintf(str, MAXSTR, "PROSODY_PITCHRANGE: %s", value==FULL?"full":"half");
  disp(cons,str);
  if(!thfTtsConfigGet(ses, tts, PROSODY_PHRASEBREAK_RESTART, &value))
    THROW("thfTtsConfigGet");
  snprintf(str, MAXSTR, "PROSODY_PHRASEBREAK_RESTART: %s",
	   value==NO?"no":"yes");
  disp(cons,str);
  if(!thfTtsConfigGet(ses, tts, TTS_SAMPLERATE, &value))
    THROW("thfTtsConfigGet");
  snprintf(str, MAXSTR, "TTS_SAMPLERATE: %s Hz", value==SR_16K?"16000":"8000");
  disp(cons,str);
  return;

 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(tts) thfTtsDestroy(tts);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  exit(1);
}


#if defined(_WIN32_WCE)
WINAPI WinMain(HINSTANCE inst, HINSTANCE previnst, LPWSTR cmdLine, int show)
#elif defined(__SYMBIAN32__)
  int SymbCMain(void *inst)
#elif defined(__PALMOS__)
#include <PalmOS.h>
#include "stdioPalm68k.h"
  UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags)
#else
#define inst NULL
  int main(int argc, char **argv)
#endif
{

  int numPhrases = 6;
  char *phrases[] = {PHRASE1, PHRASE2, PHRASE3, PHRASE3, PHRASE3, PHRASE3};
  tts_t *tts=NULL;
  thf_t *ses=NULL;
  const char *text;
  const char *ewhere, *ewhat=NULL;
  unsigned long speechLen=0;
  int i=0;
  short *speech=NULL;
  void *cons=NULL;
  char str[512], wfile[512];
  int sampleRate=SAMPLERATE;
#if FULLSYNTHESIS
#else
  unsigned  bufLen=0, frameSz=0, numFrames=0, j=0;
  short *buf=NULL;
#endif

#if STATICDATA

  static const unsigned short params[2] = {PROSODY, VOCODER};
# define INDEX 0
# include STATIC_VOICEFILE
# undef INDEX
  static const void *voicedata[] = {
    &db0,
    &auxdb0,
    &pronun0,
    &phonrules0,

#ifdef HAVE_textnorm
#  undef HAVE_textnorm
    &textnorm0,
#else
    NULL,
#endif

#ifdef HAVE_phrasebreak
#  undef HAVE_phrasebreak
    &phrasebreak0,
#else
    NULL,
#endif

#ifdef HAVE_pnorm
#  undef HAVE_pnorm
    &XXXpnorm0,
#else
    NULL, 
#endif

#ifdef HAVE_ortnormB
#  undef HAVE_ortnormB
    &ortnormB0,
#else
    NULL, 
#endif

#ifdef HAVE_gwavedb
#  undef HAVE_gwavedb
    &gwavedb0,
#else
    NULL,
#endif

#ifdef HAVE_wtoken
#  undef HAVE_wtoken
    &wtoken0,
#else
    NULL, 
#endif

    &params
  };
  sdata_t voice_sdata = {1, voicedata};
#endif

  /* Initialise console */
  if (!(cons=initConsole(inst))) return 1;

  /* Create session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

#if STATICDATA
  /* Create voice object */
  disp(cons,"Loading static voice...");
  if(!(tts=thfTtsCreateFromStatic(ses,&voice_sdata,0)))
    THROW("thfTtsCreateFromStatic");
#else
  /* Create voice object */
  disp(cons,"Loading voice file: " THF_TTSDIR VOICEFILE);
  if(!(tts=thfTtsCreateFromFile(ses,THF_TTSDIR VOICEFILE)))
    THROW("thfTtsCreateFromFile");
#endif
  displaySettings(ses, tts, cons);

  for (i=0; i<numPhrases; i++) {
    text=phrases[i];
    sprintf(str,"\nSynthesizing: %s",text);
    disp(cons,str);
    switch(i) {
    case 0:
      CC(thfTtsConfigSet(ses, tts, PROSODY_DOMAIN, NAMES));
      CC(thfTtsConfigSet(ses, tts, PROSODY_PITCHRANGE, FULL));
      CC(thfTtsConfigSet(ses, tts, PROSODY_PHRASEBREAK_RESTART, NO));
      CC(thfTtsConfigSet(ses, tts, TTS_SAMPLERATE, SR_16K));
      break;
    case 3:
      CC(thfTtsConfigSet(ses, tts, PROSODY_PITCHRANGE, HALF));
      break;
    case 4:
      CC(thfTtsConfigSet(ses, tts, PROSODY_PHRASEBREAK_RESTART, YES));
      break;
    case 5:
      CC(thfTtsConfigSet(ses, tts, TTS_SAMPLERATE, SR_8K));
      sampleRate=8000;
      break;
    }
    displaySettings(ses, tts, cons);

#if (FULLSYNTHESIS)  /* Full synthesis */  
    disp(cons,"FULL SYNTHESIS");
    if (!(speech=thfTtsSynthesize(ses, tts, text, PITCH, DURATION,
				  RATE, &speechLen)))
      THROW("thfTtsSynthesize");

    sprintf(wfile,"tts%i.wav",i+1);
    sprintf(str, "Saving %lu audio samples to %s", speechLen, wfile);
    disp(cons,str);
    if (!thfWaveSaveToFile(ses,wfile,speech,speechLen,sampleRate))
      THROW("thfWaveSaveToFile");
    thfFree(speech);

#else      /* Pipelined synthesis */  
    disp(cons,"PIPELIEND SYNTHESIS");
    if (!(numFrames=thfTtsPrep(ses, tts, text, PITCH,DURATION,RATE,&frameSz))) 
      THROW("thfTtsPrep");
    speech=malloc(numFrames*frameSz*sizeof(short));
    speechLen=0;
    for(j=0; j<numFrames; j++) {
      if (!(buf=thfTtsGetFrame(ses, tts, &bufLen))) 
	THROW("thfTtsGetFrame");
      memcpy(&speech[j*frameSz],buf,bufLen*sizeof(short));
      speechLen+=bufLen;
    }
    sprintf(wfile,"tts%i.wav",i+1);
    sprintf(str,"Saving wave: %s",wfile);
    disp(cons,str);
    if (!thfWaveSaveToFile(ses,wfile,speech,speechLen,SAMPLERATE)) 
      THROW("thfWaveSaveToFile");
    free(speech);
#endif

    thfTtsReset(tts);
  }

  /* Clean Up */
  disp(cons,"Cleaning up");
  thfTtsDestroy(tts); tts=NULL;
  thfSessionDestroy(ses); ses=NULL;

  /* Wait on user */
  disp(cons,"Done");
  closeConsole(cons);
  return 0;
  
  /* Async error handling, see console.h for panic */
 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(tts) thfTtsDestroy(tts);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1; 
}
