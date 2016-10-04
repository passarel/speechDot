/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: sRecogList.c
 * Creates a recognizer and performs recognition on the entire WAVE file.
 *  - Uses static recognizer
 *  - Uses static RIFF wave file as input
 *  - Uses pre-compiled static vocabulary built with buildList.c and 
 *    converted to static file using the staticCreateSearch.exe tool
 *---------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <datalocations.h>

#define WAVEFILE           "john_smith_16kHz" /* RIFF Wave */

#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")

#define BEAM    (200.f)
#define NBEST   (3)
#define STRSZ   (128)

#ifdef __PALMOS__
// TODO:  Move to a header
static sdata_t *precog_foo = NULL, *psearch_foo = NULL;
extern void *palmGetArmSymbol(thf_t *ses, char *fname);
static void getStatics(thf_t *ses)
{
  sdata_t **pr,**ps;
  pr = (sdata_t **)palmGetArmSymbol(ses, "recog_foo");
  precog_foo = (sdata_t*)pr;
  ps = (sdata_t **)palmGetArmSymbol(ses, "search_foo");
  psearch_foo = (sdata_t*)ps;
}
#else
extern sdata_t recog_foo;
extern sdata_t search_foo;
sdata_t *precog_foo = (sdata_t *)&recog_foo;
sdata_t *psearch_foo = (sdata_t *)&search_foo;
#endif

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }


#if defined(_WIN32_WCE)
WINAPI WinMain(HINSTANCE inst, HINSTANCE previnst, LPWSTR cmdLine, int show)
#elif defined(__SYMBIAN32__)
int SymbCMain(void *inst)
#elif defined(__PALMOS__)
#include <PalmOS.h>
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
  const char *ewhere, *ewhat=NULL;
  float score,from,to;
  unsigned short i, status;
  unsigned long inlen=0, sampleRate=0;
  float beam,garbage,any;
  void *cons=NULL;
#ifdef __PALMOS__
  void *inst = (void*)(UInt32)cmd;
#endif

  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;

  /* Create SDK session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

#ifdef __PALMOS__
  getStatics(ses);
#endif
  /* Read wave file (16-bit PCM) */
  disp(cons,"Reading wavefile: " SAMPLES_DATADIR WAVEFILE ".wav");
  if(!(thfWaveFromFile(ses,SAMPLES_DATADIR WAVEFILE ".wav",&inbuf,&inlen,
		       &sampleRate)))
    THROW(SAMPLES_DATADIR WAVEFILE ".wav");

  /* Create recognizer */
  disp(cons,"Loading recognizer...");
  if(!(r=thfRecogCreateFromStatic(ses,precog_foo,0,0xffff,0xffff,SDET))) 
    THROW("thfRecogCreateFromStatic");

  /* Create search */
  disp(cons,"Loading search..."); 
  if(!(s=thfSearchCreateFromStatic(ses,r,psearch_foo,0,NBEST))) 
    THROW("thfSearchCreateFromStatic");

  /* Configure parameters */
  beam=BEAM; garbage=0.5; any=0.5;
  if (!thfSearchConfigSet(ses,r,s,SCH_BEAM,beam))
    THROW("searchConfigSet: beam");
  if (!thfSearchConfigSet(ses,r,s,SCH_GARBAGE,garbage))
    THROW("searchConfigSet: garbage");
  if (!thfSearchConfigSet(ses,r,s,SCH_ANY,any))
    THROW("searchConfigSet: any");

  /* Initialize recognizer */
  disp(cons,"Initializing recognizer...");
  if(!thfRecogInit(ses,r,s,RECOG_KEEP_NONE)) THROW("thfRecogInit");

  /* Convert input wave samplerate (if needed) */
  if(sampleRate!=(unsigned)thfRecogGetSampleRate(ses,r)) {
    short *cspeech=NULL;
    unsigned long clen;
    if(!thfRecogSampleConvert(ses,r,sampleRate,inbuf,inlen,&cspeech,&clen))
      THROW("recogSampleConvert");
    thfFree(inbuf); 
    inbuf=cspeech;inlen=clen;
    disp(cons,"Converted samplerate");
  }

  disp(cons,"Speech detection and wave recognition...");
  if(!thfRecogPipe(ses,r,inlen,inbuf,SDET_RECOG,&status)) 
    THROW("thfRecogPipe");
  switch (status) {
  case RECOG_QUIET:   disp(cons,"No speech detected"); break;
  case RECOG_SILENCE: disp(cons,"Time out--No speech detected"); break;
  case RECOG_SOUND:   disp(cons,"Truncated speech"); break;
  case RECOG_IGNORE:  disp(cons,"Non speech sound detected"); break;
  case RECOG_DONE:    disp(cons,"Speech detected"); break;
  case RECOG_MAXREC:  disp(cons,"Too much speech"); break;
  }

  /* Report N-best recognition result */
  rres=NULL;
  if (status==RECOG_DONE || status==RECOG_MAXREC || status==RECOG_SOUND) {
    disp(cons,"Recognition result...");
    if(!thfRecogGetSpeechRange(ses,r,&from,&to)) 
      THROW("thfRecogGetSpeechRange");
    sprintf(str,"Found speech from %ld to %ld ms",(long)from,(long)to); disp(cons,str);
    { 
      float clip;
      if (!thfRecogGetClipping(ses,r,&clip)) THROW("thfRecogGetClipping");
      if (clip>1.0) disp(cons,"WARNING: Speech is clipped");
    }
    score=0;
    for(i=1;i<=NBEST;i++) {
      if (!thfRecogResult(ses,r,&score,&rres,NULL,NULL,NULL,NULL,NULL,NULL))
	THROW("recogResult");
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
  thfRecogDestroy(r); r=NULL;
  thfSearchDestroy(s); s=NULL;
  thfSessionDestroy(ses); ses=NULL;

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
