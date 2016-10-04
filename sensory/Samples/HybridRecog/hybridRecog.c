/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: hybridRecog.c
 * Compiles a hybrid grammar specification into binary search file.
 * - Hybrid grammar combines generic and task-specific digits acoustic models
 *---------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <audio.h>
#include <datalocations.h>

#define _DEBUG_PRINT_OUT 0
#define APPTITLE "Sensory Hybrid v1.0"
#define DIGITSDIR      _STR(DATADIR) "/en_us_16kHz_digits_v9/"
#define DIGITSLTSFILE "lts_en_us_digits_digit_mfcc_15_250_v6.2.raw" /* Pronunciation model */
#define DIGITSNETFILE "nn_en_us_digits_digit_mfcc_15_250_v6.2.raw" /* Acoustic model */
#define SEARCHFILE  "hybrid.raw"          /* Compiled search */
#define BEAM        (200.f)               /* Pruning beam */
#define NOTA        (0.5f)                /* Out of vocabulary threshold */
#define LSILENCE    (5000.f)              /* Maximum intial silence allowed */
#define TSILENCE    (1000.f)              /* Trailing silence required */
#define MAXRECORD   (10000.f)             /* Maximum record time */
#define MAXSTR      (512) 
#define NBEST 3


#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) { ewhere=(a); ewhat=(b); goto error; }


#ifdef _WIN32_WCE
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
  const char *ewhere=NULL, *ewhat=NULL;
  void *cons=NULL;
  float beam,nota;
  float lsil, tsil, maxR;
  char str[MAXSTR],ch;
  const char *rres;
  float score;
  unsigned short i, ii;
#if _DEBUG_PRINT_OUT // Print out status
  unsigned short x;
#endif // Print out status
  unsigned short status, done = 0, file_input = 0;
  thf_t *ses = NULL;
  recog_t *hr=NULL, *r[]={NULL,NULL};
  searchs_t *s=NULL;
  pronuns_t *hp=NULL, *p[]={NULL,NULL};
  audiol_t *pa;

  char *gra = "digit= oh%0 | zero%0 | one%1 | two%2 | three%3 | four%4 | five%5 | six%6 | seven%7 | eight%8 | nine%9; \
 digits =  $digit $digit $digit [*sil%%] $digit $digit $digit [*sil%%] $digit $digit [*sil%%] $digit $digit; \
 names = home | john smith; \
 gra = *sil%% (*nota | call ($names | $digits)) *sil%%; \
 acousticmodel: call 1; \
 acousticmodel: $gra/*sil 1; \
 acousticmodel: $gra/*nota 1; \
 acousticmodel: $names 1;";
  const char *w0="*sil",
    *w1="*nota",
    *w2="call",
    *w3="home",
    *w4="john",
    *w5="smith", 
    *w6="oh",
    *w7="zero",
    *w8="one",
    *w9="two",
    *w10="three",
    *w11="four",
    *w12="five",
    *w13="six",
    *w14="seven",
    *w15="eight",
    *w16="nine";
  const char *word1[] = {w0,w6,w7,w8,w9,w10,w11,w12,w13,w14,w15,w16}; /* digit words */
  const char *word2[] = {w0,w1,w2,w3,w4,w5}; /* generic words */
  const char **word[] = {word1,word2};  
  const char *pro0=". .pau .",
    *pro1=". .nota .",
    *pro2=". k >1 l .",
    *pro3=". h oU1 m .",
    *pro4=". dZ A1 n .",
    *pro5=". s m I1 T .";
  const char *pronun1[] = {NULL}; /* Task-specific pronunciations filled internally */
  const char *pronun2[] = {pro0,pro1,pro2,pro3,pro4,pro5};
  const char **pronun[] = {pronun1,pronun2};
  unsigned short count[] = {12,6};
  FILE *fp=NULL;

  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;
  sprintf(str,"%s\n",APPTITLE); disp(cons,str);

  /* Create SDK session */
  if (!(ses = thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

  /* SDK version */
  sprintf(str,"TrulyHandsfree SDK v%s\n",thfVersion()); disp(cons,str);

  /* Create digits recognition object */
  if(!(r[0]=thfRecogCreateFromFile(ses,DIGITSDIR DIGITSNETFILE,
				   250,0xffff,SDET)))
    THROW("thfRecogCreateFromFile");

  /* Create generic recognition object */
  if(!(r[1]=thfRecogCreateFromFile(ses,THF_DATADIR NETFILE,250,0xffff,SDET)))
    THROW("thfRecogCreateFromFile");

  /* Try to load search from file else compile one from scratch */
  if ((fp = fopen(SEARCHFILE, "r"))) {
    disp(cons,"Loading pre-built search...");
    fclose(fp);
    
    if(!(thfRecogCreateHybrid(ses,r,NULL,2,NULL,NULL,NULL,&hr,NULL)))        
      THROW("thfRecogCreateHybrid");
    if(!(s=thfSearchCreateFromFile(ses,hr,SEARCHFILE,NBEST)))
      THROW("thfSearchCreateFromFile"); 
  } else {
    disp(cons,"Building search...");
    /* Create digits pronunciation object */
    if(!(p[0]=thfPronunCreateFromFile(ses,DIGITSDIR DIGITSLTSFILE,0)))
      THROW("pronunCreateFromFile");

    /* Create generic pronunciation object */
    if(!(p[1]=thfPronunCreateFromFile(ses,THF_DATADIR LTSFILE, 0)))
      THROW("pronunCreateFromFile");

    /* Create hybrid object from collection of recog/pronun objects */
    if(!(thfRecogCreateHybrid(ses,r,p,2,word,pronun,count,&hr,&hp))) 
      THROW("CreateHybridPronun");

   /* Compile search */
    if(!(s=thfSearchCreateFromGrammar(ses,hr,hp,gra,NULL,NULL,0,NBEST,NO_PHRASESPOTTING))) 
      THROW("thfSearchCreateFromGrammar");
    
   /* Save search to file */
    if(!(thfSearchSaveToFile(ses,s,SEARCHFILE))) 
      THROW("thfSearchSaveToFile"); 
  } 

 /* Configure parameters */
  beam=BEAM; nota=NOTA;
  if (!thfSearchConfigSet(ses,hr,s,SCH_BEAM,beam))
    THROW("thfSearchConfSet: beam");
  if (!thfSearchConfigSet(ses,hr,s,SCH_NOTA,nota))
    THROW("thfSearchConfSet: nota");

  lsil=LSILENCE; tsil=TSILENCE, maxR=MAXRECORD;  
  if (!thfRecogConfigSet(ses, hr, REC_LSILENCE, lsil))
    THROW("thfRecogConfigSet: lsil");
  if (!thfRecogConfigSet(ses, hr, REC_TSILENCE, tsil))
    THROW("thfRecogConfigSet: tsil");
  if (!thfRecogConfigSet(ses, hr, REC_MAXREC, maxR))
    THROW("thfRecogConfigSet: maxrec");

  /* Initialize recognizer */
  if(!thfRecogInit(ses,hr,s,RECOG_KEEP_WAVE)) 
    THROW("thfRecogInit");
  
  //disp(cons, "Attach process for debugging, then hit <ENTER> ...");
  //do {
  //  ch = getchar();
  //} while (ch != EOF && ch != '\n');
  //clearerr(stdin);

  /* Initialize audio */
  initAudio_ex(inst, cons, &file_input);
  if (file_input)
    disp(cons, "Initialized audio for file_input ...");
  else
    disp(cons, "Initialized audio ...");

  for (ii=1; ii<=5; ii++) {
    sprintf(str,"\n===== LOOP %i of 5 ======",ii); disp(cons,str);
    disp(cons,"--------------------------------");
    disp(cons,"Vocabulary: call home, call john smith or call <10 digits>"); 
    if (file_input) {
      disp(cons, "--------------------------------");
    } else {
      disp(cons, "Press <Enter> to start");
      disp(cons,"--------------------------------");

      do {
        ch = getchar();
      } while (ch != EOF && ch != '\n');
      clearerr(stdin);
    }
    
#if _DEBUG_PRINT_OUT // Print out status
    sprintf(str, "%i. startRecord() being called", ii);
    disp(cons, str);
#endif // Print out status
    if (startRecord() == RECOG_NODATA)
      break;

    /* Pipelined recognition */
    done = 0;
#if _DEBUG_PRINT_OUT // Print out status
    x = 1;
#endif // Print out status
    while (!done) {
      audioEventLoop();
      while ((pa = audioGetNextBuffer(&done)) && !done) {
#if SHOWAMP // display max amplitude
        unsigned short maxamp=0;
        int k;
        for(k=0; k<pa->len;k++)
        if (abs(pa->audio[k])>maxamp)
          maxamp=abs(pa->audio[k]);
        sprintf(str,"samples=%d, amplitude=%d",pa->len,maxamp);
        disp(cons,str);
#endif
        if (!thfRecogPipe(ses, hr, pa->len, pa->audio, SDET_RECOG, &status))
          THROW("recogPipe");
        audioNukeBuffer(pa);
#if _DEBUG_PRINT_OUT // Print out status
        switch (status) {
        case RECOG_QUIET:
          sprintf(str, "%i.%i. status = RECOG_QUIET", ii, x);
          break;
        case RECOG_SILENCE:
          sprintf(str, "%i.%i. status = RECOG_SILENCE", ii, x);
          break;
        case RECOG_SOUND:
          sprintf(str, "%i.%i. status = RECOG_SOUND", ii, x);
          break;
        case RECOG_DONE:
          sprintf(str, "%i.%i. status = RECOG_DONE", ii, x);
          break;
        case RECOG_MAXREC:
          sprintf(str, "%i.%i. status = RECOG_MAXREC", ii, x);
          break;
        case RECOG_IGNORE:
          sprintf(str, "%i.%i. status = RECOG_IGNORE", ii, x);
          break;
        case RECOG_NODATA:
          sprintf(str, "%i.%i. status = RECOG_NODATA", ii, x);
          break;
        default:
          break;
        };
        x++;
        disp(cons, str);
#endif // Print out status
        if (status == RECOG_DONE) {
          done = 1;
          stopRecord();
        }
      }
    }

    /* Report N-best recognition result */
    rres = NULL;
    if (status == RECOG_DONE) {
      {
        float clip;
        if (!thfRecogGetClipping(ses, hr, &clip)) THROW("thfRecogGetClipping");
        if (clip>0.0) disp(cons, "WARNING: Speech is clipped");
      }
      disp(cons, "Recognition results...");
      score = 0;
      for (i = 0; i<NBEST; i++) {
        unsigned long inSpeechSamples, sdetSpeechSamples;
        const short *inSpeech, *sdetSpeech;

        if (!thfRecogResult(ses, hr, &score, &rres, NULL, NULL, &inSpeech,
          &inSpeechSamples, &sdetSpeech, &sdetSpeechSamples))
          THROW("thfRecogResult");
        if (rres == 0) break;
#ifndef __PALMOS__
        sprintf(str, "Result %i: %s (%.3f)", i + 1, rres, score); disp(cons, str);
#else
        sprintf(str, "Result %i: %s (%ld)", i + 1, rres, (long)(1000 * score + .5)); disp(cons, str);
#endif
        /* Save the recorded and speech detected audio only once */
        if (!i) {
          if (inSpeech) {
            sprintf(str, "recorded-%i.wav", ii);
            if (!thfWaveSaveToFile(ses, str, (short *)inSpeech,
              inSpeechSamples, thfRecogGetSampleRate(ses, hr)))
              THROW("thfWaveSaveToFile Recorded");
          }
          if (sdetSpeech) {
            sprintf(str, "detected-%i.wav", ii);
            if (!thfWaveSaveToFile(ses, str, (short *)sdetSpeech,
              sdetSpeechSamples, thfRecogGetSampleRate(ses, hr)))
              THROW("thfWaveSaveToFile Detected");
          }
        }
      }
    }
    else
      disp(cons, "No recognition result");
    thfRecogReset(ses, hr);
  }
  
  /* Clean up */
  if(s)thfSearchDestroy(s);
  if(hp)thfPronunDestroy(hp);   
  if(hr)thfRecogDestroy(hr);
  if(ses)thfSessionDestroy(ses);
  /* Wait on user */
  disp(cons,"Done");
  closeConsole(cons);
  return 0;

  /* Async error handling, see console.h for panic */
 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  panic(cons,ewhere,ewhat,0);
  if(s)thfSearchDestroy(s);
  if(hp)thfPronunDestroy(hp);   
  if(hr)thfRecogDestroy(hr);
  if(ses)thfSessionDestroy(ses);
  return 1; 
}
