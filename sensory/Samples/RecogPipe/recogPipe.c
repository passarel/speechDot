/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example : recogPipe.c
 * Pipelined recognition using live audio input.
 *  - Uses simple explicit audio interface
 *  - Records up to 5000ms
 *  - Illustrates pipelined recognition
 *  - Uses pre-compiled vocabulary built with buildList.c
 *---------------------------------------------------------------------------
 * IMPORTANT:
 * This SDK sample is not a suitable template for a real application.
 * Consider using separate threads for capturing audio versus performing 
 * pipelined recognition to ensure adequate cycles are available for 
 * the audio thread, otherwise you may miss audio samples. 
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <audio.h>
#include <datalocations.h>

#define SEARCHFILE    "names.raw"                     /* Compiled search */
//#define PROCESSLATER
#define BEAM           (200.f)               /* Pruning beam */
#define LSILENCE       (20000.f)             /* Maximum silence allowed */
#define MAXRECORD      (10000.f)             /* Maximum record time */
#define NBEST          (3)                  /* Max number of results */
#define STRSZ          (512)                 /* Output string size */

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
  char str[STRSZ];
  const char *ewhere, *ewhat=NULL;
  float score,from,to;
  unsigned short status, done=0;
  audiol_t *p, **buf=NULL;
  unsigned short i,ii;
#ifdef PROCESSLATER
  unsigned short j,idx;
  long bufSz=0;
#endif
  float beam,garbage,any,nota;
  float lsil, maxR;
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

  /* Create recognizer */
  disp(cons,"Loading recognizer: " THF_DATADIR NETFILE);
  if(!(r=thfRecogCreateFromFile(ses,THF_DATADIR NETFILE,(unsigned short)(AUDIO_BUFFERSZ/1000.f*SAMPLERATE),-1,SDET))) 
    THROW("thfRecogCreateFromFile");

  /* Create search */
  disp(cons,"Loading search: " SAMPLES_DATADIR SEARCHFILE);
  if(!(s=thfSearchCreateFromFile(ses,r,SAMPLES_DATADIR SEARCHFILE,NBEST))) 
    THROW("thfSearchCreateFromFile");

  /* Configure parameters */
  disp(cons,"Setting parameters...");
  beam=BEAM; garbage=0.5; any=0.5, nota=0.6f;
  if (!thfSearchConfigSet(ses,r,s,SCH_BEAM,beam))
    THROW("searchConfigSet: beam");
  if (!thfSearchConfigSet(ses,r,s,SCH_GARBAGE,garbage))
    THROW("searchConfigSet: garbage");
  if (!thfSearchConfigSet(ses,r,s,SCH_ANY,any))
    THROW("searchConfigSet: any");
  if (!thfSearchConfigSet(ses,r,s,SCH_NOTA,nota))
    THROW("searchConfigSet: nota");

  lsil=LSILENCE; maxR=MAXRECORD;
  if (!thfRecogConfigSet(ses, r, REC_LSILENCE, lsil))
    THROW("thfRecogConfigSet: lsil");
  if (!thfRecogConfigSet(ses, r, REC_MAXREC, maxR))
    THROW("thfRecogConfigSet: maxrec");

  /* Initialize recognizer */
  disp(cons,"Initializing recognizer...");
  if(!thfRecogInit(ses,r,s,RECOG_KEEP_WAVE_WORD_PHONEME)) 
    THROW("thfRecogInit");

  /* Initialize audio */
  disp(cons,"Initializing audio...");
  initAudio(inst,cons);
  
  for(ii=0; ii<5; ii++) {
    sprintf(str,"\n=====Loop %i of 5======",ii+1); disp(cons,str);
    
    disp(cons,"Recording...");
    if (startRecord() == RECOG_NODATA)
      break;
    disp(cons, ">>> SPEAK NOW <<<");
    
    /* Pipelined recognition */
    done=0;
#ifdef PROCESSLATER
    idx=0;
#endif
    while (!done) {
      audioEventLoop();
      while((p=audioGetNextBuffer(&done)) && !done) {
#ifdef PROCESSLATER
	if(idx+1>=bufSz) {
	  bufSz+=10; // +10 buffers
	  buf=realloc(buf,bufSz*sizeof(p));
	}
	buf[idx]=p; // save audio for later processing
	idx++;
	if(!thfRecogPipe(ses,r,p->len,p->audio,SDET_ONLY,&status))
	  THROW("thfRecogPipe");
#else
	if(!thfRecogPipe(ses,r,p->len,p->audio,SDET_RECOG,&status))
	  THROW("recogPipe");
	audioNukeBuffer(p);
#endif
	switch (status) {
	case RECOG_QUIET:   disp(cons,"quiet"); break;
	case RECOG_SOUND:   disp(cons,"sound");break;
	case RECOG_IGNORE:  disp(cons,"ignored");break;
	case RECOG_SILENCE: disp(cons,"timeout: silence");break;
	case RECOG_MAXREC:  disp(cons,"timeout: maxrecord"); break;
	case RECOG_DONE:    disp(cons,"end-of-speech");break;
	default:            disp(cons,"other");break;
	}
	if(status!=RECOG_QUIET && status!=RECOG_SOUND) {
	  done=1;
	  stopRecord();
	  disp(cons,"Stopped recording");
	}
      }
    }
    
    /* Report N-best recognition result */
    rres=NULL;
    if (status==RECOG_DONE || status==RECOG_MAXREC) {
      thfRecogGetSpeechRange(ses,r,&from,&to);
      sprintf(str,"Found speech from %ld to %ld ms",(long)from,(long)to);disp(cons,str);
#ifdef PROCESSLATER
      disp(cons,"Delayed recognition...");
      {
	unsigned short i,fromI,toI;
	fromI=(int)from*SAMPLERATE/1000; toI=(int)to*SAMPLERATE/1000;
	i=0;
	for(j=0;j<idx;j++) {
	  p=buf[j];
	  if (i+p->len > fromI && i < toI) {
	    /* k = length of buffer to pass
	       j = position in buffer to pass */
	    unsigned short k = (i + p->len > toI) ? toI - i:p->len;
	    unsigned short j = (i>fromI) ? 0 : fromI - i;
	    if (k < p->len) status=RECOG_DONE;
	    else status=RECOG_SOUND;
	    if(!thfRecogPipe(ses,r,(unsigned short)(k-j),&p->audio[j],
			     RECOG_ONLY,&status))
	      THROW("thfRecogPipe");
	  } 
	  i+=p->len;
	  audioNukeBuffer(p);
	}
      }
#endif
      disp(cons,"Recognition results...");
      {
	float clip;
	if (!thfRecogGetClipping(ses,r,&clip)) THROW("thfRecogGetClipping");
	if (clip>0.0) disp(cons,"WARNING: Speech is clipped");
      }
      score=0;
      for(i=0; i<NBEST; i++) {
	const char *walign, *palign;
	unsigned long inSpeechSamples, sdetSpeechSamples;
	const short *inSpeech, *sdetSpeech;
	
	if (!thfRecogResult(ses,r,&score,&rres,&walign,&palign,&inSpeech,
			    &inSpeechSamples,&sdetSpeech,&sdetSpeechSamples)) 
	  THROW("thfRecogResult");
	if (rres==0) break; 
#ifndef __PALMOS__
        sprintf(str,"Result %i: %s (%.3f)",i+1,rres,score); disp(cons,str);
#else
        sprintf(str,"Result %i: %s (%ld)",i+1,rres,(long)(1000*score+.5)); disp(cons,str);
#endif
	
	/* Save the recorded and speech detected audio only once */
	if(!i) {
	  if(inSpeech)
	    if(!thfWaveSaveToFile(ses,"recorded.wav",(short *)inSpeech,
				  inSpeechSamples,SAMPLERATE)) 
	      THROW("thfWaveSaveToFile Recorded");
	  if(sdetSpeech)
	    if(!thfWaveSaveToFile(ses,"detected.wav",(short *)sdetSpeech,
				  sdetSpeechSamples,SAMPLERATE)) 
	      THROW("thfWaveSaveToFile Detected");
	}
	/* save word alignments */
	if(walign) {
	  sprintf(str,"result_%i_best.wrd",i+1);
	  if(!(fp=fopen(str,"wt"))) THROW("Opening word alignment file");
	  fprintf(fp,"MillisecondsPerFrame: 1\nEND OF HEADER\n");
	  fwrite(walign,1,strlen(walign),fp);
	  fclose(fp); fp=NULL;
	}
	/* save phoneme alignments */
	if(palign) {
	  sprintf(str,"result_%i_best.phn",i+1);
	  if(!(fp=fopen(str,"wt"))) THROW("Opening word alignment file");
	  fprintf(fp,"MillisecondsPerFrame: 1\nEND OF HEADER\n");
	  fwrite(palign,1,strlen(palign),fp);
	  fclose(fp); fp=NULL;
	}
      } 
    } else 
      disp(cons,"No recognition result");
    thfRecogReset(ses,r);
  }
  /* Clean up */
  killAudio();
  if(buf)free(buf); buf=NULL;
  thfRecogDestroy(r); r=NULL;
  thfSearchDestroy(s); s=NULL;
  thfSessionDestroy(ses); ses=NULL;
  disp(cons,"Done");
  closeConsole(cons);
  return 0;

  /* Async error handling, see console.h for panic */
 error:
  killAudio();
  if(buf)free(buf); buf=NULL;
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(fp) fclose(fp);
  if(r) thfRecogDestroy(r);
  if(s) thfSearchDestroy(s);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1;
}
