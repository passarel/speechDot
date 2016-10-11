/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: PhraseSpot.c
 *---------------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <audio.h>
#include <datalocations.h>

#define MODE_LOAD_CUSTOM  (1)  // Load pre-built custom HBG vocabulary from file (uses custom HBG acoustic model)
#define MODE_BUILD_MANUAL (2)  // Build vocabulary with explicit phrasespot parameters (uses gerenic acoustic model)
#define MODE_BUILD_AUTO   (3)  // Build vocabulary with automatic phrasespot parameters (uses generic acoustic model)

#define BUILD_MODE        (MODE_LOAD_CUSTOM)
   
#define GENDER_DETECT (1)  // Enable/disable gender detection

#define HBG_GENDER_MODEL "hbg_genderModel.raw"
#define HBG_NETFILE      "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_am.raw"
#define HBG_SEARCHFILE   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_9.raw" // pre-built search
#define HBG_OUTFILE      "myhbg.raw"           /* hbg output search */

#define NBEST              (1)                /* Number of results */
#define PHRASESPOT_PARAMA_OFFSET   (0)        /* Phrasespotting ParamA Offset */
#define PHRASESPOT_BEAM    (100.f)            /* Pruning beam */
#define PHRASESPOT_ABSBEAM (100.f)            /* Pruning absolute beam */
#if GENDER_DETECT
# define PHRASESPOT_DELAY  15                 /* Phrasespotting Delay */
#else
# define PHRASESPOT_DELAY  PHRASESPOT_DELAY_ASAP  /* Phrasespotting Delay */
#endif
#define MAXSTR             (512)              /* Output string size */
#define SHOWAMP            (0)                /* Display amplitude */

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }


char *formatExpirationDate(time_t expiration)
{
  static char expdate[33];
  if (!expiration) return "never";
  strftime(expdate, 32, "%m/%d/%Y 00:00:00 GMT", gmtime(&expiration));
  return expdate;
}

#if defined(__iOS__)
int Phrasespot(void *inst)
#else
int main(int argc, char **argv)
#define inst NULL
#endif
{
  const char *rres;
  thf_t *ses=NULL;
  recog_t *r=NULL;
  searchs_t *s=NULL;
  pronuns_t *pp=NULL;
  char str[MAXSTR];
  const char *ewhere, *ewhat=NULL;
  float score, delay;
  unsigned short status, done=0;
  audiol_t *p;
  unsigned short i=0;
  void *cons=NULL;
  FILE *fp=NULL;

#if BUILD_MODE>MODE_LOAD_CUSTOM
  float paramAOffset, beam, absbeam;

#if BUILD_MODE==MODE_BUILD_AUTO
  int j=0;
  unsigned short phraseCount = 2;
  const char *p0="Hello Blue Genie", *p1="This is a test";
  const char *phrase[] = {p0,p1};

#else
  /* NOTE: Phrasespot parameters are phrase specific. The following paramA,paramB,paramC values are tuned specifically for 'hello blue genie'. It is strongly recommended that you determine optimal parameters empirically if you change vocabulary. Please note that Sensory offers a service to provide tuned pre-built search files for custom phrasespot vocabularies. Tuned vocabularies provided by Sensory contain additional tuning information which may yield better recognition accuracy than those built manually using thfSearchCreateFromGrammar or thfPhrasespotCreateFromList. */

  const char *gra = "g=hello blue genie; \
      paramA:$g -850; \
      paramB:$g 740; \
      paramC:$g 226;";
  unsigned short wordCount = 3;
  const char *w0="hello",*w1="blue", *w2="genie";
  const char *word[] = {w0,w1,w2};
  const char *p0=". h,45,90 (^ | E) . l oU1 .",
    *p1=". b l u1 .",
    *p2=". dZ i1 . n i .";
  const char *pronun[] = {p0,p1,p2};
#endif
#endif

  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;

  /* Create SDK session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

  /* Display TrulyHandsfree SDK library version number */
  dispv(cons, "TrulyHandsfree SDK Library version: %s\n", thfVersion());
  dispv(cons, "Expiration date: %s\n\n",
	formatExpirationDate(thfGetLicenseExpiration()));

  /* Create recognizer */
#if BUILD_MODE==MODE_LOAD_CUSTOM
  /* NOTE: HBG_NETFILE is a custom model for 'hello blue genie'. Use a generic acoustic model if you change vocabulary or contact Sensory for assistance in developing a custom acoustic model specific to your vocabulary. */
  disp(cons,"Loading recognizer: "  HBG_NETFILE);
  //if(!(r=thfRecogCreateFromFile(ses, SAMPLES_DATADIR HBG_NETFILE,(unsigned short)(AUDIO_BUFFERSZ/1000.f*SAMPLERATE),-1,NO_SDET)))
  if(!(r=thfRecogCreateFromFile(ses, HBG_NETFILE,(unsigned short)(AUDIO_BUFFERSZ/1000.f*SAMPLERATE),-1,NO_SDET)))
    THROW("thfRecogCreateFromFile");

#else

  disp(cons,"Loading recognizer: "  NETFILE);
  if(!(r=thfRecogCreateFromFile(ses, THF_DATADIR NETFILE,(unsigned short)(AUDIO_BUFFERSZ/1000.f*SAMPLERATE),-1,NO_SDET))) 
    THROW("thfRecogCreateFromFile");

#endif


 #if BUILD_MODE>MODE_LOAD_CUSTOM

 /* Create pronunciation object */
  disp(cons,"Loading pronunciation model: " LTSFILE);
  if(!(pp=thfPronunCreateFromFile(ses,THF_DATADIR LTSFILE,0)))
    THROW("pronunCreateFromFile");

#if BUILD_MODE==MODE_BUILD_AUTO
  
  // v3.7.0 Experimental code : automatically selects phrasespot parameters

 /* Create search */ 
  if(!(s=thfPhrasespotCreateFromList(ses,r,pp,(const char**)phrase,phraseCount,NULL,NULL,0,PHRASESPOT_LISTEN_CONTINUOUS))) 
    THROW("PhrasespotCreateFromList");

#else
 
  /* Create search */
  disp(cons,"Compiling search...");
  if(!(s=thfSearchCreateFromGrammar(ses,r,pp,gra,word,pronun,wordCount,NBEST,PHRASESPOTTING))) 
    THROW("thfSearchCreateFromGrammar");

#endif

  /* Configure parameters */
  disp(cons,"Setting parameters..."); 
  paramAOffset=PHRASESPOT_PARAMA_OFFSET; 
  beam=PHRASESPOT_BEAM; 
  absbeam=PHRASESPOT_ABSBEAM; 
  delay=PHRASESPOT_DELAY;
  if (!thfPhrasespotConfigSet(ses,r,s,PS_PARAMA_OFFSET,paramAOffset))
    THROW("thfPhrasespotConfigSet: parama_offset");
  if (!thfPhrasespotConfigSet(ses,r,s,PS_BEAM,beam))
    THROW("thfPhrasespotConfigSet: beam");
  if (!thfPhrasespotConfigSet(ses,r,s,PS_ABSBEAM,absbeam))
    THROW("thfPhrasespotConfigSet: absbeam");
  if (!thfPhrasespotConfigSet(ses,r,s,PS_DELAY,delay))
    THROW("thfPhrasespotConfigSet: delay");

   /* Save search */
  disp(cons,"Saving search: " SAMPLES_DATADIR HBG_OUTFILE);  
  if(!(thfSearchSaveToFile(ses,s,SAMPLES_DATADIR HBG_OUTFILE))) 
    THROW("thfSearchSaveToFile");

#else
  //disp(cons,"Loading custom HBG search: " SAMPLES_DATADIR HBG_SEARCHFILE);
  disp(cons,"Loading custom HBG search: " HBG_SEARCHFILE);
  //if(!(s=thfSearchCreateFromFile(ses,r,SAMPLES_DATADIR HBG_SEARCHFILE,NBEST)))
  if(!(s=thfSearchCreateFromFile(ses,r, HBG_SEARCHFILE,NBEST)))
    THROW("thfSearchCreateFromFile");

  /* Configure parameters - only DELAY... others are saved in search already */
  disp(cons,"Setting parameters...");  
  delay=PHRASESPOT_DELAY;
  if (!thfPhrasespotConfigSet(ses,r,s,PS_DELAY,delay))
    THROW("thfPhrasespotConfigSet: delay");
#endif

  if (thfRecogGetSampleRate(ses,r) != SAMPLERATE)
    THROW("Acoustic model is incompatible with audio samplerate");


  /* Initialize recognizer */
  disp(cons,"Initializing recognizer...");

#ifndef GENDER_DETECT
  if(!thfRecogInit(ses,r,s,RECOG_KEEP_NONE))
    THROW("thfRecogInit");

#else
  if(!thfRecogInit(ses,r,s,RECOG_KEEP_WAVE_WORD_PHONEME))
    THROW("thfRecogInit");

  /* initialize a speaker object with a single speaker,
   * and arbitrarily set it up
   * to hold one recording from this speaker
   */
  if (!thfSpeakerInit(ses,r,0,1)) {
    THROW("thfSpeakerInit");
  }
  
  disp(cons,"Loading gender model: " HBG_GENDER_MODEL);
  /* now read the gender model, which has been tuned to the target phrase */
  //if (!thfSpeakerReadGenderModel(ses,r,SAMPLES_DATADIR HBG_GENDER_MODEL)) {
  if (!thfSpeakerReadGenderModel(ses,r,HBG_GENDER_MODEL)) {
    THROW("thfSpeakerReadGenderModel");
  }
#endif

  /* Initialize audio */
  disp(cons,"Initializing audio...");
  initAudio(inst,cons);

  for (i=1; ; i++) {
    //sprintf(str,"\n===== LOOP %i of 5 ======",i); disp(cons,str);
   // disp(cons,"Recording...");
    if (startRecord() == RECOG_NODATA)
      break;

#if BUILD_MODE==MODE_BUILD_AUTO
    disp(cons,"Say a command :");
    for(j=0; j<phraseCount; j++) {
      sprintf(str," - %s",phrase[j]);
      disp(cons, str);   
    }
#else
    //disp(cons,">>> Say 'Hello Blue Genie' <<<");
#endif

    /* Pipelined recognition */
    done=0;
    while (!done) {
      audioEventLoop();
      while ((p = audioGetNextBuffer(&done)) && !done) {
#if SHOWAMP // display max amplitude
        unsigned short maxamp=0;
        int k;
        for(k=0; k<p->len;k++)
          if (abs(p->audio[k])>maxamp)
            maxamp=abs(p->audio[k]);
        sprintf(str,"samples=%d, amplitude=%d",p->len,maxamp);
        disp(cons,str);
#endif
        if (!thfRecogPipe(ses, r, p->len, p->audio, RECOG_ONLY, &status))
          THROW("recogPipe");
        audioNukeBuffer(p);
        if (status == RECOG_DONE) {
          done = 1;
          stopRecord();
        }
      }
    }
    
    /* Report N-best recognition result */
    rres=NULL;
    if (status==RECOG_DONE) {
      //disp(cons,"Recognition results...");
      score=0;
      if (!thfRecogResult(ses,r,&score,&rres,NULL,NULL,NULL,NULL,NULL,NULL)) 
        THROW("thfRecogResult");
      sprintf(str,"Result: !!!%s (%.3f)",rres,score); disp(cons,str);

#if GENDER_DETECT
      {
        float genderProb;
        if (!thfSpeakerGender(ses, r, &genderProb)) {
          THROW("thfSpeakerGender");
        }
        if (genderProb < 0.5) {
          sprintf(str, "Gender: MALE (score=%f)\n", (1.0 - genderProb));
        }
        else {
          sprintf(str, "Gender: FEMALE (score=%f)\n", genderProb);
        }
        disp(cons, str);
      }
#endif

    } else 
      disp(cons,"No recognition result");
    thfRecogReset(ses,r);
  }
  /* Clean up */
  killAudio();
  thfRecogDestroy(r); r=NULL;
  thfSearchDestroy(s); s=NULL;
  thfPronunDestroy(pp); pp=NULL;
  thfSessionDestroy(ses); ses=NULL;
  disp(cons,"Done");
  closeConsole(cons);

  return 0;

  /* Async error handling, see console.h for panic */
 error:
  killAudio();
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(fp) fclose(fp);
  if(r) thfRecogDestroy(r);
  if(s) thfSearchDestroy(s);
  if(pp) thfPronunDestroy(pp);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1;
}
