/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: speakerIdentification.c
 *---------------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <audio.h>
#include <datalocations.h>  // Specified which language pack to use

#define BUILDSEARCH  (0)   // Control 'build' versus 'load' search from file

/* Hello Blue Genie data files */
#define HBG_NETFILE       "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_am.raw" /* acoustic model */
#define HBG_SEARCHFILE1   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_3.raw" /* pre-built search (Tight = Accepts less) */
#define HBG_SEARCHFILE2   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_5.raw" 
#define HBG_SEARCHFILE3   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_7.raw" 
#define HBG_SEARCHFILE4   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_9.raw" /* pre-built search (Medium = Accepts medium) */
#define HBG_SEARCHFILE5   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_11.raw" 
#define HBG_SEARCHFILE6   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_13.raw" 
#define HBG_SEARCHFILE7   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_15.raw"  /* pre-built search (Loose = Accepts more) */  

#define OUTSEARCHFILE "mysearch.raw"           /* output search */
#define NBEST              (1)             /* Number of results */
#define PHRASESPOT_PARAMA_OFFSET   (0)     /* Phrasespotting ParamA Offset */
#define PHRASESPOT_BEAM    (100.f)         /* Pruning beam */
#define PHRASESPOT_ABSBEAM (100.f)         /* Pruning absolute beam */
#define PHRASESPOT_DELAY   (250)	   /* Phrasespotting Delay: at least 250ms needed for speaker ID */
#define MAXSTR             (512)           /* Output string size */
#define SHOWAMP            (0)             /* Display amplitude */

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }

#if defined(__iOS__)
int SpeakerIdentification(void *inst)
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
  float score;
  unsigned short status, done=0;
  audiol_t *p;
  float delay;
  void *cons=NULL;
  FILE *fp=NULL;
  char *prompt="Hello Blue Genie";
  unsigned short numRepetitions = 3;
  unsigned short numUsers = 2;
  unsigned short rIdx, uIdx, vIdx;
  float rawScore, bestScore;
  unsigned short bestUser;
    
#if BUILDSEARCH
  /* NOTE: Phrasespot parameters are phrase specific. The following paramA,paramB,paramC values are tuned specifically for 'hello blue genie'. It is strongly recommended that you determine optimal parameters empirically if you change vocabulary. */
  float paramAOffset, beam, absbeam;
  char *gra = "g=hello blue genie; \
    paramA:$g -850;		   \
    paramB:$g 740;		   \
    paramC:$g 226;";
  unsigned short count = 3;
  const char *w0="hello",*w1="blue", *w2="genie";
  const char *word[] = {w0,w1,w2};
  const char *p0=". h E . l oU1 .",
    *p1=". b l u1 .",
	*p2=". dZ i1 . n i .";
    const char *pronun[] = {p0,p1,p2};
#endif
    
  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;
  
  /* Create SDK session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }
  
  /* Create recognizer */
  /* NOTE: HBG_NETFILE is a custom model for 'hello blue genie'. Use a generic acoustic model if you change vocabulary or contact Sensory for assistance in developing a custom acoustic model specific to your vocabulary. */
  disp(cons,"Loading recognizer: " HBG_NETFILE); 
  if(!(r=thfRecogCreateFromFile(ses,SAMPLES_DATADIR HBG_NETFILE,(unsigned short)(AUDIO_BUFFERSZ/1000.f*SAMPLERATE),-1,NO_SDET)))
    THROW("thfRecogCreateFromFile");
  
#if BUILDSEARCH
  
  /* Create pronunciation object */
  disp(cons,"Loading pronunciation model: " LTSFILE);
  if(!(pp=thfPronunCreateFromFile(ses,THF_DATADIR LTSFILE,0)))
    THROW("pronunCreateFromFile");
  /* Create search */
  disp(cons,"Compiling search...");
  if(!(s=thfSearchCreateFromGrammar(ses,r,pp,gra,word,pronun,count,NBEST,PHRASESPOTTING))) 
    THROW("thfSearchCreateFromGrammar");
  
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
  disp(cons,"Saving search: " OUTSEARCHFILE);  
  if(!(thfSearchSaveToFile(ses,s,SAMPLES_DATADIR OUTSEARCHFILE))) 
    THROW("thfSearchSaveToFile");
#else
  disp(cons,"Loading default search: " HBG_SEARCHFILE4);  
  if(!(s=thfSearchCreateFromFile(ses,r,SAMPLES_DATADIR HBG_SEARCHFILE4,NBEST)))
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
  if(!thfRecogInit(ses,r,s,RECOG_KEEP_WAVE_WORD_PHONEME))
    THROW("thfRecogInit");
  
  /* Initialize audio */
  disp(cons,"Initializing audio...");
  initAudio(inst,cons);
    
  // loop over all users to get enrollment recordings, and then
  // loop one more time to do speaker ID.
  for (uIdx = 1; uIdx <= numUsers+1; uIdx++) {
    disp(cons,"\n\n");
    // if it's the first user, then initialize the speaker object
    if (uIdx == 1) {
      if (!thfSpeakerInit(ses,r,uIdx,numRepetitions)) {
	THROW("thfSpeakerInit");
      }
    }
    // if it's a valid subsequent user, then add user to speaker object
    else if (uIdx <= numUsers) {
      if (!thfSpeakerAdd(ses,r,uIdx)) {
	THROW("thfSpeakerAdd");
      }
    } 
    // if we have enrolled all users, do adaptation and switch to 'ID' mode
    else { // (uIdx > numUsers) 
      disp(cons,"\n==== Learning Users ====\n");
      for (vIdx = 1; vIdx <= numUsers; vIdx++) {
	sprintf(str,"\tLearning user %d...", vIdx);
	disp(cons,str);
	if (!thfSpeakerAdapt(ses,r,vIdx,0,"IGNORE_SV_DEFAULT",512)) {
	  THROW("thfSpeakerAdapt");
	}
      }
      disp(cons,"\n");
      numRepetitions = 5;
    }
        
    // elicit 'numRepetitions' recordings from the current user
    for (rIdx = 1; rIdx <= numRepetitions; rIdx++) {
      if (uIdx <= numUsers) {
	sprintf(str,"\n===== USER %d, enroll %d of %d =====\n", uIdx, rIdx, numRepetitions);
	disp(cons,str);
	sprintf(str,"User %d: say '%s'", uIdx, prompt);
	disp(cons,str);
      }
      else {
	sprintf(str,"\n===== Identify User: %d of %d =====\nSay '%s'", rIdx,numRepetitions, prompt);
	disp(cons,str);
      }
            
      if (startRecord() == RECOG_NODATA)
        break;
            
      /* Pipelined recognition */
      done=0;
      while (!done) {
	audioEventLoop();
	while((p=audioGetNextBuffer(&done)) && !done) {
#if SHOWAMP // display max amplitude
	  unsigned short maxamp=0;
	  int k;
	  for(k=0; k<p->len;k++)
	    if (abs(p->audio[k])>maxamp)
	      maxamp=abs(p->audio[k]);
	  sprintf(str,"samples=%d, amplitude=%d",p->len,maxamp);
	  disp(cons,str);
#endif
	  if(!thfRecogPipe(ses,r,p->len,p->audio,RECOG_ONLY,&status))
	    THROW("recogPipe");
	  audioNukeBuffer(p);
	  if(status==RECOG_DONE) {
	    done=1;
	    stopRecord();
	  }
	}
      }
            
      /* Report 1-best recognition result */
      rres=NULL;
      if (status==RECOG_DONE) {
	score=0;
	if (!thfRecogResult(ses,r,&score,&rres,NULL,NULL,NULL,
			    NULL,NULL,NULL)) 
	  THROW("thfRecogResult");
	sprintf(str,"Result: %s (%.3f)",rres,score); 
	disp(cons,str);
                
	// either use this recording for enrollment, or for speaker ID
	if (uIdx <= numUsers) {
	  if (!thfSpeakerStoreLastRecording(ses,r,uIdx,(int)(score*4096.0f),1)) {
	    THROW("thfSpeakerStoreLastRecording");
	  }
	}
	else {
	  bestScore = 0.0f;
	  bestUser = 1;
	  for (vIdx = 1; vIdx <= numUsers; vIdx++) {
	    if (!thfSpeakerVerify(ses,r,vIdx,&rawScore,NULL)) {
	      THROW("thfSpeakerVerify");
	    }
	    if (rawScore >= bestScore) {
	      bestScore = rawScore;
	      bestUser = vIdx;
	    }
	  }
	  sprintf(str,"ID: USER %d (score %f)\n", bestUser, bestScore);
	  disp(cons,str);
	}
      } 
      else {
	disp(cons,"No recognition result");
      }
      thfRecogReset(ses,r);
    }
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


