/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: speakerVerification.c
 *---------------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
# define NEEDS_GETOPT
# define snprintf _snprintf
#else
# include <unistd.h>
#endif

#include <trulyhandsfree.h>

#ifdef __ANDROID__
# include "common.h"
# define DISP(cons, text) disp(inst, job, text)
# define PATH(i,a) strcat(strcpy(tmpFileName[i], appDir), a)
# define START_REC() startRecord(inst, jrecord)
# define STOP_REC()  stopRecord(inst, jrecord)
# define GET_NEXT_BUFFER() audioGetNextBuffer(inst, jrecord)
#else
# include <console.h>
# include <audio.h>
# include <datalocations.h>  // Specifies which language pack to use
# define DISP(cons, text) disp(cons, text)
# define PATH(i,a) a
# define START_REC() startRecord()
# define STOP_REC()  stopRecord()
# define GET_NEXT_BUFFER() audioGetNextBuffer(&done)
char *checkFlagsText(int flags);
#endif

#define HBG_RECOGFILE   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_am.raw"
#define HBG_SEARCHFILE  "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_15.raw" 
#define HBG_SVFILE      "hbg_antidata_v6_0.raw"
#define HBG_SVSPOT_THRESHOLD (0.0f)  /* SVspot threshold is vocabulary-specific and determined empirically */
#define HBG_SV_THRESHOLD (0.3f)  /* SV threshold is vocabulary-specific and determined empirically */
#define MY_HBG_RECOGFILE "my_hbg_net.raw"         /* Output acoustic model */
#define MY_HBG_SEARCHFILE "my_hbg_search.raw"     /* Output search */

#define EMBEDDED_TARGET     "in36"         /* Target DSP used when saving embedded data files */
#define EMBEDDED_SEARCHFILE "myEmbeddedSearch.bin"   /* combined trigger search for all users */
#define EMBEDDED_RECOGFILE  "myEmbeddedRecog.bin"    /* combined trigger recognizer for all users */

#define ALWAYS_ENROLL      (1)
#define SAVE_RECORDINGS    (1)

#define NUM_USERS          (1)				/* NOTE: SV only supports a single user */
#define NUM_ENROLL         (3)
#define NUM_TEST           (3)
#define BLOCKSZ            (2400)           /* 10 frames at 16kHz   */
#define NBEST              (1)				/* Number of results  */
#define MAXSTR             (512)			/* Output string size */
#define PHRASESPOT_DELAY   (15)
#define TURN_EPQ_OFF       (0)
#define SDET_LSILENCE      (60000.f)
#define SDET_TSILENCE      (500.f)
#define SDET_MAXRECORD     (60000.f)
#define SDET_MINDURATION   (500.f)

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }


static void freeUserData(thfUser_t *user, int numUsers)
{
  int eIdx, uIdx;
  for (uIdx = 0; uIdx < numUsers; uIdx++) {
    if (user[uIdx].userName) free(user[uIdx].userName); 
    user[uIdx].userName=NULL;
    for (eIdx=0; eIdx<user[uIdx].numEnroll; eIdx++) {
      if (user[uIdx].enroll[eIdx].audio)  free(user[uIdx].enroll[eIdx].audio);
      user[uIdx].enroll[eIdx].audio=NULL;
      if (user[uIdx].enroll[eIdx].filename) free(user[uIdx].enroll[eIdx].filename);
      user[uIdx].enroll[eIdx].filename=NULL;
    }
    if (user[uIdx].enroll != NULL)free(user[uIdx].enroll);
    user[uIdx].enroll=NULL;
  }
  free(user); user=NULL;
}


#if defined(__iOS__)
int SpeakerVerification(void *inst)
#elif defined(__ANDROID__)
jlong Java_com_sensory_sdk_Console_SpeakerVerification
(JNIEnv* inst, jobject job, jstring jappDir, jobject jrecord)
#else
int main(int argc, char **argv)
# define inst NULL
#endif
{
  const char *rres;
  thf_t *ses=NULL;
  recog_t *recogSpeechDetect=NULL; 
  recog_t *recogSV=NULL;
  searchs_t *searchSV=NULL;
  recog_t *recogEnroll=NULL;
  searchs_t *searchEnroll=NULL;
  recog_t *recogTest=NULL;
  searchs_t *searchTest=NULL;
  thfUser_t *user = NULL;
  char str[MAXSTR];
  const char *ewhere, *ewhat=NULL;
  float score;
  unsigned short status, done=0;
  void *cons=NULL;
  audiol_t *aud=NULL;
  FILE *fp=NULL;
  unsigned short numUsers = NUM_USERS, numEnroll = NUM_ENROLL, numTest=NUM_TEST; 
  unsigned short uIdx, eIdx; 
  unsigned long globalFeedback;
  unsigned long *feedbackArray = NULL;
  float phraseQuality = 0.0f;
  float delay=PHRASESPOT_DELAY;
  float lsil=SDET_LSILENCE, tsil=SDET_TSILENCE, maxR=SDET_MAXRECORD, minDur=SDET_MINDURATION;
  int i;
  unsigned long inLen;
  unsigned int blockSize=BLOCKSZ;
  char *phrase="HELLO BLUE GENIE";
#ifdef __ANDROID__
	const char *appDir = (*inst)->GetStringUTFChars(inst, jappDir, 0);
	char tmpFileName[3][MAXSTR];
#endif

  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;
  
  /* Create SDK session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }  
  
#ifdef __ANDROID__
  displaySdkVersionAndExpirationDate(inst, job);
#endif
  
  /* Initialize audio */
  DISP(cons,"Initializing audio...");
  initAudio(inst, cons);
  STOP_REC(); // initAudio() starts recording immediately, which we don't want right now
  
  /* if not always enrolling, try to load existing search and recognizer from file */
  if (!ALWAYS_ENROLL) {
    recogTest=thfRecogCreateFromFile(ses,PATH(0,MY_HBG_RECOGFILE),0xFFFF,-1,NO_SDET);
    if (recogTest != NULL)
      searchTest=thfSearchCreateFromFile(ses,recogTest,PATH(0,MY_HBG_SEARCHFILE),NBEST);
    if (recogTest != NULL && searchTest == NULL) {
      thfRecogDestroy(recogTest);
      recogTest = NULL;
    }
  }

  /* if we always enroll, or if reading from file failed, then enroll */
  if (ALWAYS_ENROLL || recogTest == NULL || searchTest == NULL) {
    /* --------- initialization ------ */
    
    /* create and initialize user structure */
    user = malloc(sizeof(thfUser_t) * numUsers);
    if(!user)THROW("out of memory");
    memset(user, 0, sizeof(thfUser_t) * numUsers);
    for (uIdx = 0; uIdx < numUsers; uIdx++) {
      user[uIdx].userName = malloc(sizeof(char) * MAXSTR);
      snprintf(user[uIdx].userName, MAXSTR-1, "user%d", uIdx+1);
      user[uIdx].numEnroll = 0;
      user[uIdx].enroll = NULL;
    }
    
    /* create feedback array for all users */
    feedbackArray = malloc(sizeof(unsigned long) * numEnroll);
    if(!feedbackArray)THROW("out of memory");
    memset(feedbackArray, 0, sizeof(unsigned long) * numEnroll);

    /* Recognizer - used to speech detect input audio */
    if(!(recogSpeechDetect=thfRecogCreateFromFile(ses,PATH(0,SAMPLES_DATADIR HBG_RECOGFILE),0xFFFF,-1,SDET_ONLY))) 
      THROW("thfRecogCreateFromFile");    
    if (!thfRecogConfigSet(ses, recogSpeechDetect, REC_LSILENCE, lsil))
      THROW("thfRecogConfigSet: lsil");
    if (!thfRecogConfigSet(ses, recogSpeechDetect, REC_TSILENCE, tsil))
      THROW("thfRecogConfigSet: tsil");
    if (!thfRecogConfigSet(ses, recogSpeechDetect, REC_MAXREC, maxR))
      THROW("thfRecogConfigSet: maxrec");
    if (!thfRecogConfigSet(ses, recogSpeechDetect, REC_MINDUR, minDur))
      THROW("thfRecogConfigSet: mindur");
    if(!thfRecogInit(ses, recogSpeechDetect, NULL, RECOG_KEEP_WAVE))
      THROW("thfPhonemeRecogInit");


    /* Recognizer - used to enroll recordings  */
    if(!(recogEnroll=thfRecogCreateFromFile(ses,PATH(0,SAMPLES_DATADIR HBG_RECOGFILE),0xFFFF,-1,NO_SDET))) 
      THROW("thfRecogCreateFromFile");
    if(!(searchEnroll=thfSearchCreateFromFile(ses,recogEnroll,PATH(0,SAMPLES_DATADIR HBG_SEARCHFILE),NBEST)))
      THROW("thfSearchCreateFromFile");
    if(!thfRecogInit(ses, recogEnroll, searchEnroll,
		     RECOG_KEEP_WAVE_WORD_PHONEME))
      THROW("thfRecogInit");
    if(!thfSpeakerCreateFromFile(ses, recogEnroll, NUM_ENROLL, PATH(0,SAMPLES_DATADIR HBG_SVFILE)))
      THROW("thfSpeakerCreateFromFile");
    if (!thfPhrasespotConfigSet(ses,recogEnroll,searchEnroll,PS_DELAY,delay))
      THROW("thfPhrasespotConfigSet: delay");


    /* --------- get all enrollment recordings and learn enrollments ---------*/
    /* get and save recordings from each user */
    for (uIdx = 0; uIdx < numUsers; uIdx++) {
      sprintf(str,"\n===== USER %d =====\n", uIdx+1);
      
      /* Record enroll phrases for each user */
      eIdx = 0;
      while (eIdx < numEnroll) {
	sprintf(str,"\n===== User %d: Enroll %d of %d =====\n", uIdx+1, eIdx+1, numEnroll);
	DISP(cons,str);
	
	/* Pipelined recognition */
	if (START_REC() == RECOG_NODATA)
	  break;

	sprintf(str,"Say: %s",phrase); DISP(cons,str);
	done=0;
	rres=NULL;
	while (!done) {   /* Feeds audio into recognizer until sdet done */
	  audioEventLoop();
	  while((aud=GET_NEXT_BUFFER()) && !done) {
	    if(!thfRecogPipe(ses,recogSpeechDetect,aud->len,aud->audio,SDET_ONLY,&status))
	      THROW("recogPipe");
	    switch (status) {
	    case RECOG_QUIET:   DISP(cons,"quiet"); break;
	    case RECOG_SOUND:   DISP(cons,"sound");break;
	    case RECOG_IGNORE:  DISP(cons,"ignored");break;
	    case RECOG_SILENCE: DISP(cons,"timeout: silence");break;
	    case RECOG_MAXREC:  DISP(cons,"timeout: maxrecord"); break;
	    case RECOG_DONE:    DISP(cons,"end-of-speech");break;
	    default:            DISP(cons,"other");break;
	    }
	    audioNukeBuffer(aud);
	    if(status==RECOG_DONE || status==RECOG_MAXREC 
	       || status==RECOG_SILENCE) {
	      done=1;
	      STOP_REC();
	    }
	  }
	}
   	if (status==RECOG_DONE || status==RECOG_MAXREC) {
	  const short *sdetSpeech;
	  unsigned long sdetSpeechLen, j;
	  short *buf=NULL;
	  unsigned long bufLen;

	  if (!thfRecogResult(ses,recogSpeechDetect,&score,&rres,NULL,NULL,NULL,NULL,&sdetSpeech,&sdetSpeechLen)) 
	    THROW("thfRecogResult");
	  sprintf(str,"***SPEECH DETECTED: length=%li",sdetSpeechLen);
	  DISP(cons,str);

	  // Copy sdetSpeech buffer because it's owned by SDK
	  buf=malloc((sizeof(short)*sdetSpeechLen));
	  memcpy(buf,sdetSpeech,sdetSpeechLen*sizeof(short));
	  bufLen=sdetSpeechLen;	     

	  // Feed speech detected audio through 'Hello Blue Genie' phrasespotter
	  rres=NULL;
	  for (j=0; j<bufLen; j+=blockSize) {
	    if ((inLen=bufLen-j) > blockSize) inLen=blockSize;
	    if(!thfRecogPipe(ses,recogEnroll,inLen,(short*)&buf[j],RECOG_ONLY,&status))
	      THROW("recogPipe");
	    if (status==RECOG_DONE) {
	      if (!thfRecogResult(ses,recogEnroll,&score,&rres,NULL,NULL,NULL,NULL,NULL,NULL)) 
		THROW("thfRecogResult");
	      sprintf(str,"***ENROLL: res=%s (score=%f)\n",rres,score);
	      DISP(cons,str);
	      break;
	    }
	  }
	  if(rres) {
	    // Check individual recording
	    // NOTE: a real application may want to 'redo' problem recordings
	    thfCheckRecording(ses, recogEnroll, (short *)buf, bufLen, &globalFeedback);
	    if (globalFeedback != 0) {
	      sprintf(str,"Warning: detected poor recording quality. Code=0x%x (%s)", (unsigned int)globalFeedback, checkFlagsText(globalFeedback));
	      DISP(cons,str);
	    }
	    
	    user[uIdx].numEnroll += 1;
	    user[uIdx].enroll = realloc(user[uIdx].enroll, sizeof(thfEnrollment_t) * user[uIdx].numEnroll);
	    user[uIdx].enroll[eIdx].audio = malloc(bufLen * sizeof(short));
	    memcpy(user[uIdx].enroll[eIdx].audio, buf, bufLen * sizeof(short));
	    user[uIdx].enroll[eIdx].audioLen = bufLen;
	    user[uIdx].enroll[eIdx].filename = malloc(sizeof(char) * MAXSTR);
	    snprintf(user[uIdx].enroll[eIdx].filename, MAXSTR-1, "user%u_enroll%u.wav", uIdx+1, eIdx+1);
	    
#if SAVE_RECORDINGS
	    if (buf) {
	      DISP(cons,"Saving wave...");
	      DISP(cons,user[uIdx].enroll[eIdx].filename);
	      if (!thfWaveSaveToFile(ses, PATH(0,user[uIdx].enroll[eIdx].filename),
				     (short *)buf,bufLen,SAMPLERATE))
		THROW("thfWaveSaveToFile");
	    }
#endif
	    eIdx++;
  
	  } else {
	    DISP(cons,"No recognition result");
	  }
	  if(buf)free(buf);buf=NULL;

	} else {
	  DISP(cons,"End of speech not found");
	}
	thfRecogReset(ses,recogSpeechDetect);
	thfRecogReset(ses,recogEnroll);
      }
      
      /* check set of recordings */
      if(!thfSpeakerCheckEnrollments(ses, recogEnroll, user, uIdx, &globalFeedback, feedbackArray,&phraseQuality))
	THROW("thfSpeakerCheckEnrollments");
      /* display feedback on set of recordings */
      /* check feedback; if non-zero, display warning about particular recording(s) */
      sprintf(str,"Phrase quality: %.2f\n", phraseQuality);
      DISP(cons,str);
      if (globalFeedback != 0) {
	for (eIdx = 0; eIdx < numEnroll; eIdx++) {
	  if (feedbackArray[eIdx] != 0) {
	    sprintf(str,"Warning: poor recording detected for user %d, enrollment %d. Code=0x%x (%s)",
		    uIdx+1, eIdx+1, (unsigned int)feedbackArray[eIdx],
		    checkFlagsText(feedbackArray[eIdx]));
	    DISP(cons,str);
	  }
	}
      }
    }

    /* show how to set some parameter values; see documentation of thfSpeakerEnroll() to see relevant parameters */
    if (!thfPhrasespotConfigSet(ses, recogEnroll, searchEnroll, PS_PARAMA_OFFSET, 1200))
      THROW("thfPhrasespotConfigSet");
	if (!thfSpeakerConfigSet(ses, recogEnroll, 0, SPEAKER_TRIGGER_SAMP_PER_CAT, 512))
      THROW("thfSpeakerConfigSet");

    DISP(cons,"\nLearning enrollments from all users...");
    /* --------- Learn enrollments for all users ------ */
    
    if(!thfSpeakerEnroll(ses, recogEnroll, NULL, "IGNORE_SV_DEFAULT", (unsigned short)numUsers, user, &searchSV,&recogSV))
      THROW("thfSpeakerEnroll");

    /* EPQ is turned on after calling thfSpeakerEnroll(), but user may want to turn it off */
#if TURN_EPQ_OFF
    if (!thfRecogConfigSet(ses,recogSV,REC_EPQ_ENABLE,0))
      THROW("thfRecogConfigSet");
#endif
    
    if(!thfSpeakerSaveRecogToFile(ses, searchSV, recogSV, PATH(0,MY_HBG_SEARCHFILE), PATH(1,MY_HBG_RECOGFILE)))
      THROW("thfRecogAndSearchSaveToFile");
    
    // Optional: save recognizer and search in deeply embedded format
    if(!thfSaveEmbedded(ses, recogSV, searchSV, EMBEDDED_TARGET,PATH(0,EMBEDDED_RECOGFILE), PATH(1,EMBEDDED_SEARCHFILE), 0, 1))
      THROW("thfSaveEmbedded");
    
    // Load from file so they are correctly initialized
    thfRecogDestroy(recogSV); recogSV=NULL;
    thfSearchDestroy(searchSV); searchSV=NULL;
    if(!(recogTest=thfRecogCreateFromFile(ses,PATH(0,MY_HBG_RECOGFILE),0xFFFF,-1,NO_SDET)))
      THROW("thfRecogCreateFromFile");
    if(!(searchTest=thfSearchCreateFromFile(ses,recogTest,PATH(0,MY_HBG_SEARCHFILE),NBEST)))
      THROW("thfSearchCreateFromFile");

    freeUserData(user,numUsers); user=NULL;
  }
  

  /* --------- Test phase ---------*/
  if (!thfPhrasespotConfigSet(ses,recogTest,searchTest,PS_DELAY,delay))
    THROW("thfPhrasespotConfigSet: delay");
  
  if(!thfRecogInit(ses,recogTest,searchTest,RECOG_KEEP_WAVE_WORD_PHONEME))
    THROW("thfRecogInit");

  DISP(cons,"Testing...");
  /* Record enroll phrases for each user */
  for (i = 0; i < numTest; i++) {
    sprintf(str,"\n===== Test %d of %d =====\n", i+1, numTest);
    DISP(cons,str);
    
    /* Pipelined recognition */
    done=0;
    rres=NULL;
    if (START_REC() == RECOG_NODATA)
      break;
    sprintf(str,"Say user-defined password"); DISP(cons,str);
    while (!done) {   /* Feeds audio into recognizer until sdet done */
      audioEventLoop();
      while((aud=GET_NEXT_BUFFER()) && !done) {
	if(!thfRecogPipe(ses,recogTest,aud->len,aud->audio,RECOG_ONLY,&status))
	  THROW("recogPipe");
	switch (status) {
	case RECOG_QUIET:   DISP(cons,"quiet"); break;
	case RECOG_SOUND:   DISP(cons,"sound");break;
	case RECOG_IGNORE:  DISP(cons,"ignored");break;
	case RECOG_SILENCE: DISP(cons,"timeout: silence");break;
	case RECOG_MAXREC:  DISP(cons,"timeout: maxrecord"); break;
	case RECOG_DONE:    DISP(cons,"end-of-speech");break;
	default:            DISP(cons,"other");break;
	}
	audioNukeBuffer(aud);
	if(status==RECOG_DONE) {
	  done=1;
	  STOP_REC();
	}
      }
    }
    
    if (status==RECOG_DONE) {
      float svScore;      
      if (!thfRecogResult(ses,recogTest,&score,&rres,NULL,NULL,NULL,NULL,NULL,NULL)) 
	THROW("thfRecogResult");	  
      if (rres) {
	sprintf(str,"Result: %s",rres); 
	DISP(cons,str);
	
	svScore = thfRecogSVscore(ses, recogTest);
	sprintf(str,"SV score: %f", svScore);
	DISP(cons,str);
	
        /* demonstrate that SVspot threshold can be used to filter     */
        /* out false accepts, and SV threshold can be used for speaker */
        /* verification */
	if (svScore >= HBG_SVSPOT_THRESHOLD) {
          if (svScore >= HBG_SV_THRESHOLD) {
	    sprintf(str, "USER ACCEPTED %s\n", rres);
          } else {
	    sprintf(str, "USER REJECTED %s\n", rres);
          }
	  DISP(cons, str);
	} else {
	  sprintf(str, "WORD REJECTED %s\n", rres);
	  DISP(cons, str);
	}
      } else {
	DISP(cons,"No recognition result");
      }
    }
    thfRecogReset(ses,recogTest);
    
  }
  
  /* Clean up */
  DISP(cons, "Cleaning up..."); 
  killAudio();
  if (feedbackArray)free(feedbackArray);feedbackArray = NULL;
  if (recogEnroll) thfRecogDestroy(recogEnroll); recogEnroll=NULL;
  if (searchEnroll) thfSearchDestroy(searchEnroll); searchEnroll=NULL;
  if (recogTest) thfRecogDestroy(recogTest); recogTest=NULL;
  if (searchTest) thfSearchDestroy(searchTest); searchTest=NULL;
  thfSessionDestroy(ses); ses=NULL;
  DISP(cons,"Done");
  closeConsole(cons);
  
  return 0;
  
  /* Async error handling, see console.h for panic */
 error:
  killAudio();
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(fp) fclose(fp);
  if(user)freeUserData(user,numUsers);
  if (feedbackArray)free(feedbackArray);feedbackArray = NULL;
  if (recogEnroll) thfRecogDestroy(recogEnroll); recogEnroll=NULL;
  if (searchEnroll) thfSearchDestroy(searchEnroll); searchEnroll=NULL;
  if (recogSV) thfRecogDestroy(recogSV); recogSV=NULL;
  if (searchSV) thfSearchDestroy(searchSV); searchSV=NULL;
  if (recogTest) thfRecogDestroy(recogTest); recogTest=NULL;
  if (searchTest) thfSearchDestroy(searchTest); searchTest=NULL;
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1;
}
