/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: recogGrammar.c 
 * Creates a recognizer and performs recognition on a speech file.
 *  - Uses RAW PCM audio file as input
 *  - Uses pre-compiled vocabulary built with buildGrammar.c
 *  - Illustrates pipelined recognition
 *  - Illustrates how to calibrate speech detector
 *---------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <datalocations.h>

#define SEARCHFILE    "time.raw"                         /* Compiled search */
#define WAVEFILE      "12oclock_16kHz"                   /* RAW PCM audio   */

#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")

#define BEAM    (200.f) 
#define NBEST   (3)
#define BLOCKSZ (2000)   /* 250 ms blocksize */
#define STRSZ   (128)

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) { ewhere=(a); ewhat=(b); goto error; }


void checkEndian(unsigned short inlen,short *inbuf)
{
  unsigned short i=1;
  unsigned char *p=(unsigned char *)&i;
#ifndef __PALMOS__
  if (*p==1) return; /* little endian */
#else
  if (*p==0) return; /* big endian, don't swap */
#endif
  for (i=0, p=(unsigned char *)inbuf; i<inlen; i++, p+=2) {
    unsigned char c;
    c = *p; *p = *(p+1); *(p+1) = c; 
  }
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
  const char *rres;
  thf_t *ses=NULL;
  recog_t *r=NULL;
  searchs_t *s=NULL;
  short *inbuf=NULL;
  char str[STRSZ];
  float score, from, to;
  unsigned short i, status;
  unsigned short inlen;
  FILE *fp=NULL;
  float beam, garbage, any;
  const char *ewhere, *ewhat=NULL;
  void *cons=NULL;
#ifdef __PALMOS__
  void *inst = (void*)(UInt32)cmd;
#endif

  /* Draw Console */
  if (!(cons=initConsole(inst))) return 1;

  /* Check if data files exist */
  if(!(fp=fopen(SAMPLES_DATADIR SEARCHFILE,"rb"))) {
    panic(cons,"Missing data file","Use 'BuildGrammar' Sample to create file: " SAMPLES_DATADIR SEARCHFILE,0);
    return 1;
  }
  fclose(fp); fp=NULL;

  /* Create SDK session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

  /* Create recognizer */
  disp(cons,"Loading recognizer: " THF_DATADIR NETFILE);
  if(!(r=thfRecogCreateFromFile(ses,THF_DATADIR NETFILE,BLOCKSZ,-1,SDET)))
    THROW("recogCreateFromFile");

  /* Create search */
  disp(cons,"Loading search: " SAMPLES_DATADIR SEARCHFILE);
  if(!(s=thfSearchCreateFromFile(ses,r,SAMPLES_DATADIR SEARCHFILE,NBEST))) 
    THROW("searchCreateFromFile");

  /* Initialize recognizer */
  disp(cons,"Initializing recognizer...");
  if(!thfRecogInit(ses,r,s,RECOG_KEEP_NONE)) THROW("thfRecogInit");

  /* Configure parameters (optional) */
  beam=BEAM; garbage=0.5; any=0.5;
  if (!thfSearchConfigSet(ses,r,s,SCH_BEAM,beam))
    THROW("searchConfigSet: beam");
  if (!thfSearchConfigSet(ses,r,s,SCH_GARBAGE,garbage))
    THROW("searchConfigSet: garbage");
  if (!thfSearchConfigSet(ses,r,s,SCH_ANY,any))
    THROW("searchConfigSet: any");
  
  /* Feed speech to recognizer in chunks until done 
     -- Simulates pipelined recognition. */
  disp(cons,"Recognizing...");
  /* Open speech file (Raw 16-bit PCM) */
  if(!(fp=fopen(SAMPLES_DATADIR WAVEFILE ".audio","rb"))) 
    THROW2("Failed to open audio file",SAMPLES_DATADIR WAVEFILE ".audio");
  inbuf=malloc(BLOCKSZ*sizeof(short));

  for(inlen=0; !feof(fp);) {
    inlen = (unsigned short)fread(inbuf,sizeof(short),BLOCKSZ,fp);
    if(inlen>0) {       
      checkEndian(inlen,inbuf);
      if(!thfRecogPipe(ses,r,inlen,inbuf,SDET_RECOG,&status))
	THROW("recogPipe");
      switch (status) {
      case RECOG_QUIET:   disp(cons,"quiet"); break;
      case RECOG_SILENCE: disp(cons,"silence"); break;
      case RECOG_SOUND:   disp(cons,"speech"); break;
      case RECOG_IGNORE:  disp(cons,"ignore"); break;
      case RECOG_DONE:    disp(cons,"done"); break;
      case RECOG_MAXREC:  disp(cons,"maxrec"); break;
      }
      if(status!=RECOG_QUIET && status!=RECOG_SOUND) break;
    }
  }
  fclose(fp); fp=NULL;
  /* Report speech detector state */
  switch (status) {
  case RECOG_QUIET:   disp(cons,"No speech detected"); break;
  case RECOG_SILENCE: disp(cons,"Time out -- not speech detected"); break;
  case RECOG_SOUND:   disp(cons,"Truncated speech"); break;
  case RECOG_IGNORE:  disp(cons,"Non speech sound detected"); break;
  case RECOG_DONE:    disp(cons,"Speech detected"); break;
  case RECOG_MAXREC:  disp(cons,"Too much speech"); break;
  }
 
  /* Report N-best recognition result */
  rres=NULL;
  if (status==RECOG_DONE || status==RECOG_MAXREC || status==RECOG_SOUND) {
    thfRecogGetSpeechRange(ses,r,&from,&to);
    sprintf(str,"Found speech from %ld to %ld ms",(long)from,(long)to); disp(cons,str);
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
  free(inbuf);
  thfSearchDestroy(s);
  thfRecogDestroy(r);
  thfSessionDestroy(ses);

  disp(cons,"Done");
  closeConsole(cons);
  return 0;

  /* Async error handling, see console.h for panic */
 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(fp) fclose(fp);
  if(inbuf) free(inbuf);
  if(s) thfSearchDestroy(s);
  if(r) thfRecogDestroy(r);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1; 
}
