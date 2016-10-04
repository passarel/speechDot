/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: recogList.c
 * Creates a recognizer and performs recognition on the entire WAVE file.
 *  - Uses RIFF wave file as input
 *  - Uses pre-compiled vocabulary built with buildList.c
 *  - Shows how to calibrate speech detector
 *---------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <datalocations.h>

#define SEARCHFILE "names.raw"                     /* Compiled search */
#define WAVEFILE   "john_smith_16kHz"              /* RIFF Wave       */

#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")

#define BEAM    (200.f)
#define NBEST   (3)
#define STRSZ   (128) 

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }

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
  const char *rres;
  thf_t *ses=NULL;
  recog_t *r=NULL;
  searchs_t *s=NULL;
  short *inbuf=NULL;
  char str[STRSZ];
  float score, from, to, beam, garbage, any, nota;
  unsigned short i, status=RECOG_NODATA;
  unsigned long inlen=0, sampleRate=0;
  const char *ewhere, *ewhat=NULL;
  void *cons=NULL;
  FILE *fp=NULL;
#ifdef __PALMOS__
  void *inst = (void*)(UInt32)cmd;
#endif

  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;

  /* Check if data files exist */
  if(!(fp=fopen(SAMPLES_DATADIR SEARCHFILE,"rb"))) {
    panic(cons,"Missing data file","Use 'BuildList' Sample to create file: " SAMPLES_DATADIR SEARCHFILE,0);
    return 1;
  }
  fclose(fp); fp=NULL;

  /* Create SDK session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

  /* Read wave file (16-bit PCM) */
  disp(cons,"Reading wavefile: " SAMPLES_DATADIR WAVEFILE ".wav");
  if(!(thfWaveFromFile(ses,SAMPLES_DATADIR WAVEFILE ".wav",&inbuf,&inlen,
		       &sampleRate))) THROW(SAMPLES_DATADIR WAVEFILE ".wav");
  /* Create recognizer */
  disp(cons,"Loading recognizer: " THF_DATADIR NETFILE);
  if(!(r=thfRecogCreateFromFile(ses,THF_DATADIR NETFILE,0xFFFF,-1,SDET)))
    THROW("thfRecogCreateFromFile");
  
  /* Create search */
  disp(cons,"Loading search: " SAMPLES_DATADIR SEARCHFILE);   
  if(!(s=thfSearchCreateFromFile(ses,r,SAMPLES_DATADIR SEARCHFILE,NBEST)))
    THROW("thfSearchCreateFromFile");
 
  /* Configure parameters */
  beam=BEAM; garbage=0.5; any=0.5; nota=0.5f;
  if (!thfSearchConfigSet(ses,r,s,SCH_BEAM,beam))
    THROW("searchConfigSet: beam");
  if (!thfSearchConfigSet(ses,r,s,SCH_GARBAGE,garbage))
    THROW("searchConfigSet: garbage");
  if (!thfSearchConfigSet(ses,r,s,SCH_ANY,any))
    THROW("searchConfigSet: any");
  if (!thfSearchConfigSet(ses,r,s,SCH_NOTA,nota))
    THROW("searchConfigSet: nota");

  /* Initialize recognizer */
  disp(cons,"Initializing recognizer...");
  if (!thfRecogInit(ses,r,s,RECOG_KEEP_NONE)) THROW("thfRecogInit");
    
  /* Convert input wave samplerate (if needed) */
  if(sampleRate!=(unsigned)thfRecogGetSampleRate(ses,r)) {
    short *cspeech=NULL;
    unsigned long clen;
    if(!thfRecogSampleConvert(ses,r,sampleRate,inbuf,inlen,&cspeech,&clen))
      THROW("recogSampleConvert");
    thfFree(inbuf); 
    inbuf=cspeech;
    inlen=clen;
    disp(cons,"Converted samplerate");
  }

  /* Passes entire wave to recognizer as a single buffer */
  disp(cons,"Speech detection and wave recognition...");
  if(!thfRecogPipe(ses,r,inlen,inbuf,SDET_RECOG,&status))
    THROW("thfRecogPipe");
  switch (status) {
  case RECOG_QUIET:         disp(cons,"No speech detected");           break;
  case RECOG_SILENCE:       disp(cons,"Time out--No speech detected"); break;
  case RECOG_SOUND:         disp(cons,"Truncated speech");             break;
  case RECOG_IGNORE:        disp(cons,"Non speech sound detected");    break;
  case RECOG_DONE:          disp(cons,"Speech detected");              break;
  case RECOG_MAXREC:        disp(cons,"Too much speech");              break;
  }

 /* Report N-best recognition result */
  rres=NULL;
  if (status==RECOG_DONE || status==RECOG_MAXREC || status==RECOG_SOUND) {
    disp(cons,"Recognition result...");
    thfRecogGetSpeechRange(ses,r,&from,&to);
    sprintf(str,"Found speech from %ld to %ld ms",(long)from,(long)to); disp(cons,str);
    { 
      float clip;
      if (!thfRecogGetClipping(ses,r,&clip)) THROW("recogGetClipping");
      if (clip>1.0) disp(cons,"WARNING: Speech is clipped");
    }
    score=0;
    for(i=1;i<=NBEST;i++) {
      if(!thfRecogResult(ses,r,&score,&rres,NULL,NULL,NULL,NULL,NULL,NULL)) 
	THROW("thfRecogResult");
      if (rres==0) break; 
#ifndef __PALMOS__
      sprintf(str,"Result %i: %s (%.3f)",i,rres,score); disp(cons,str);
#else
      sprintf(str,"Result %i: %s (%ld)",i,rres,(long)(1000*score+.5)); disp(cons,str);
#endif
    }
  } else 
    disp(cons,"No recognition result");

  /* Clean up */
  thfFree(inbuf); inbuf=NULL;
  thfSearchDestroy(s); s=NULL;
  thfRecogDestroy(r); r=NULL;
  thfSessionDestroy(ses);ses=NULL;

  /* Wait on user */
  disp(cons,"Done");
  closeConsole(cons);
  return 0;

 /* Async error handling, see console.h for panic */
 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(inbuf) thfFree(inbuf);  
  if(s)     thfSearchDestroy(s);
  if(r)     thfRecogDestroy(r);
  panic(cons,ewhere,ewhat,0);
  if(ses)   thfSessionDestroy(ses);
  return 1; 
}
