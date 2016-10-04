/*
 * Sensory Confidential
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: udtsid.c
 * This example shows demonstrates the enrollment and testing phases of 
 * user-defined triggers (UDT) with speaker identication (SID).
 * It can be optionally configured to perform UDT only.
 *
 *---------------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <trulyhandsfree.h>

#ifdef __ANDROID__
# include "common.h"
# define DISP(cons, text) disp(inst, job, text)
# define PATH(i,a) strcat(strcpy(tmpFileName[i], appDir), a)
# define START_REC() startRecord(inst, jrecord)
# define STOP_REC()  stopRecord(inst, jrecord)
# define GET_NEXT_BUFFER() audioGetNextBuffer(inst, jrecord)
#else
# include "console.h"
# include "audio.h"
# include "datalocations.h"  // Specifies which language pack to use
# define DISP(cons, text) disp(cons, text)
# define PATH(i,a) a
# define START_REC() startRecord()
# define STOP_REC()  stopRecord()
# define GET_NEXT_BUFFER() audioGetNextBuffer(&done)
char *checkFlagsText(int flags);
#endif

#ifdef _WIN32
# define snprintf _snprintf
#endif

#define TRIGGER_SEARCHFILE "mySearch.raw"   /* combined trigger search for all users */
#define TRIGGER_RECOGFILE  "myRecog.raw"    /* combined trigger recognizer for all users */

#define TARGET              "in36"         /* Target used when saving embedded data files */
#define EMBEDDED_SEARCHFILE "myEmbeddedSearch.bin"   /* combined trigger search for all users */
#define EMBEDDED_RECOGFILE  "myEmbeddedRecog.bin"    /* combined trigger recognizer for all users */

#define UDT_CACHEFILE       "udtCache.udt"

#define UDTSID             (1)              /* mode: 0=UDT only, 1=UDT+SID */
#define ALWAYS_ENROLL      (1)  
#define SAVE_RECORDINGS    (1)
#define UDT_CACHE          (0)

#define NUM_USERS          (2)
#define NUM_ENROLL         (3)
#define NUM_TEST           (5)

#define NBEST              (1)             /* Number of results  */
#define PHRASESPOT_DELAY   (250)	       /* Delay >=250ms for SV/SID with multiple users*/
#define TURN_EPQ_OFF       (0)

#define MAXSTR             (512)           /* Output string size */
#define SDET_LSILENCE      (60000.f)
#define SDET_TSILENCE      (500.f)
#define SDET_MAXRECORD     (60000.f)
#define SDET_MINDURATION   (500.f)
#define SV_THRESHOLD       (0.35)

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }


#if defined(__iOS__)
int udtsid(void *inst)
#elif defined(__ANDROID__)
jlong Java_com_sensory_sdk_Console_udtsid(JNIEnv* inst,
		jobject job, jstring jappDir, jobject jrecord)
#else
int main(int argc, char **argv)
# define inst NULL
#endif
{
	const char *ewhere, *ewhat = NULL;
	thf_t *ses = NULL;
	recog_t *enrollRecog = NULL;  	 // phoneme recognizer used for enrollment
	phsearch_t *enrollSearch = NULL; // phoneme search used for enrollment
	recog_t *UDTrecog = NULL;        // recognizer created for UDT and saved to file
	searchs_t *UDTsearch = NULL;	 // search created for UDT and saved to file
	recog_t *recogTrigger = NULL;    // recognizer for UDT read from file; same as UDTrecog and used only for clarity
	searchs_t *searchTrigger = NULL; // search for UDT read from file; same as UDTsearch and used only for clarity
	thfUser_t *user = NULL;
	const char *rres = NULL;
	char str[MAXSTR];
	unsigned short status, existingCache = 0, done = 0;
	audiol_t *aud = NULL;
	void *cons = NULL;
	FILE *fp = NULL;
	float delay = PHRASESPOT_DELAY, lsil = SDET_LSILENCE, tsil = SDET_TSILENCE,
			maxR = SDET_MAXRECORD, minDur = SDET_MINDURATION;
	unsigned short numUsers = NUM_USERS;
	unsigned short numEnroll = NUM_ENROLL;
	unsigned short eIdx, uIdx;
	int i;
	unsigned long globalFeedback;
	unsigned long *feedbackArray = NULL;
	float phraseQuality = 0.0f;
	int res;
	udt_t *udt = NULL;
  struct stat st;

#ifdef __ANDROID__
	const char *appDir = (*inst)->GetStringUTFChars(inst, jappDir, 0);
	char tmpFileName[3][MAXSTR];
#endif

	/* Draw console */
	if (!(cons = initConsole(inst) ))
		return 1;

  //{
  //  disp(cons, "Attach process for debugging, then hit <ENTER> ...");
  //  char ch;
  //  do {
  //    ch = getchar();
  //  } while (ch != EOF && ch != '\n');
  //  clearerr(stdin);
  //}
  
  /* Create SDK session */
	if (!(ses = thfSessionCreate())) {
		panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
		return 1;
	}
#ifdef __ANDROID__
	displaySdkVersionAndExpirationDate(inst, job);
#endif

	/* Initialize audio */
	DISP(cons, "Initializing audio...");
	initAudio(inst, cons);
	STOP_REC(); // initAudio() starts recording immediately, which we don't want right now

	/* if not always enrolling, try to load existing search and recognizer from file */
	if (!ALWAYS_ENROLL) {
		recogTrigger = thfRecogCreateFromFile(ses, PATH(0,TRIGGER_RECOGFILE),
				0xFFFF, -1, NO_SDET);
		if (recogTrigger != NULL )
			searchTrigger = thfSearchCreateFromFile(ses, recogTrigger,
					PATH(0,TRIGGER_SEARCHFILE), NBEST);
		if (recogTrigger != NULL && searchTrigger == NULL ) {
			thfRecogDestroy(recogTrigger);
			recogTrigger = NULL;
		}
	}

	/* if we always enroll, or if reading from file failed, then enroll */
	if (ALWAYS_ENROLL || recogTrigger == NULL || searchTrigger == NULL ) {
		/* --------- initialization ------ */
    
		/* create and initialize user structure */
		user = malloc(sizeof(thfUser_t) * numUsers);
		if(!user)THROW("Out of memory");
		memset(user, 0, sizeof(thfUser_t) * numUsers);
		for (uIdx = 0; uIdx < numUsers; uIdx++) {
			user[uIdx].userName = malloc(sizeof(char) * MAXSTR);
			snprintf(user[uIdx].userName, MAXSTR - 1, "user%d", uIdx + 1);   
		  user[uIdx].numEnroll = 0;
			user[uIdx].enroll = NULL;
		}

		/* create feedback array for all users */
		feedbackArray = malloc(sizeof(unsigned long) * numEnroll);
		if(!feedbackArray)THROW("Out of memory");
		memset(feedbackArray, 0, sizeof(unsigned long) * numEnroll);

		DISP(cons, "Loading UDT data...");
		/* create UDT object */
		if (!(udt = thfUdtCreate(ses, PATH(0,THF_DATADIR NETFILE),
				PATH(1,THF_DATADIR PHNSEARCH_FILE),
        PATH(2,THF_DATADIR SVSID_FILE), numEnroll, SAMPLERATE))) {
		  THROW("thfUdtCreate");
		}

    if (UDT_CACHE)  {
      res = stat(PATH(0,UDT_CACHEFILE), &st);
      if (res != -1) existingCache = 1;

      if (existingCache)  {
		    for (uIdx = 0; uIdx < numUsers; uIdx++) {
		      user[uIdx].numEnroll = numEnroll;
  			  user[uIdx].enroll = realloc(user[uIdx].enroll,sizeof(udtEnrollment_t) * numEnroll);
          memset(user[uIdx].enroll, 0, sizeof(udtEnrollment_t) * numEnroll);
		    }
        if (!thfUdtCacheLoadFromFile(ses, user,  NUM_USERS, udt, PATH(0,UDT_CACHEFILE))) {
          THROW("thfUdtCacheLoadFromFile");
        }
      }
    }

	  /* Recognizer - used for speech detection of enrollment phrases */
    if ((!UDT_CACHE) || (!existingCache)) {
	    if (!(enrollRecog = thfRecogCreateFromFile(ses,
			    PATH(0,THF_DATADIR NETFILE), 0xFFFF, -1, SDET)))
		    THROW("thfRecogCreateFromFile");
	    if(!(enrollSearch=thfPhonemeSearchCreateFromFile(ses,enrollRecog,PATH(0,THF_DATADIR PHNSEARCH_FILE),NBEST)))
		      THROW("thfPhonemeSearchCreateFromFile");

	    if (!thfRecogConfigSet(ses, enrollRecog, REC_LSILENCE, lsil))
	      THROW("thfRecogConfigSet: lsil");
	    if (!thfRecogConfigSet(ses, enrollRecog, REC_TSILENCE, tsil))
	      THROW("thfRecogConfigSet: tsil");
	    if (!thfRecogConfigSet(ses, enrollRecog, REC_MAXREC, maxR))
	      THROW("thfRecogConfigSet: maxrec");
	    if (!thfRecogConfigSet(ses, enrollRecog, REC_MINDUR, minDur))
	      THROW("thfRecogConfigSet: mindur");

	    if (!thfPhonemeRecogInit(ses, enrollRecog, enrollSearch, RECOG_KEEP_WAVE))
		    THROW("thfPhonemeRecogInit");
    }
	  /* --------- get all enrollment recordings and learn enrollments ---------*/

	  /* get recordings from each user */
    for (uIdx = 0; uIdx < numUsers; uIdx++) {
      if ((!UDT_CACHE) || (!existingCache)){
		    sprintf(str, "\n===== USER %d =====\n", uIdx + 1);

		    /* Record enroll phrases for each user */
		    for (eIdx = 0; eIdx < numEnroll; eIdx++) {
			    sprintf(str, "\n===== User %d: Enroll %d of %d =====\n",
					    uIdx + 1, eIdx + 1, numEnroll);
			    DISP(cons, str);

			    /* Pipelined recognition */
			    if (START_REC() == RECOG_NODATA)
				    break;

			    /* Pipelined recognition */
			    done = 0;
			    rres = NULL;
			    sprintf(str, "Say user-defined password");
			    DISP(cons, str);
			    while (!done) { /* Feeds audio into recognizer until sdet done */
				    audioEventLoop();
				    while ((aud = GET_NEXT_BUFFER()) && !done) {
					    if (!thfRecogPipe(ses, enrollRecog, aud->len, aud->audio,
							    SDET_RECOG, &status))
						    THROW("recogPipe");
					    switch (status) {
					    case RECOG_QUIET:
						    DISP(cons, "quiet");
						    break;
					    case RECOG_SOUND:
						    DISP(cons, "sound");
						    break;
					    case RECOG_IGNORE:
						    DISP(cons, "ignored");
						    break;
					    case RECOG_SILENCE:
						    DISP(cons, "timeout: silence");
						    break;
					    case RECOG_MAXREC:
						    DISP(cons, "timeout: maxrecord");
						    break;
					    case RECOG_DONE:
						    DISP(cons, "end-of-speech");
						    break;
					    default:
						    break;
					    }
					    audioNukeBuffer(aud);
					    aud = NULL;
					    if (status == RECOG_DONE) {
						    done = 1;
						    STOP_REC();
					    }
				    }
				    if (aud) audioNukeBuffer(aud);
			    }

			    // Save enrollment recording
			    if (status == RECOG_DONE) {
				    const short *sdetSpeech;
				    unsigned long sdetSpeechLen;
				    float score = 0;

				    if (!thfRecogResult(ses, enrollRecog, &score, &rres, NULL,
						    NULL, NULL, NULL, &sdetSpeech, &sdetSpeechLen))
					    THROW("thfRecogResult");
				    if (rres) {
					    // check individual recording
					    thfCheckRecording(ses, enrollRecog, (short *) sdetSpeech,
							    sdetSpeechLen, &globalFeedback);
					    if (globalFeedback != 0) {
						    sprintf(str,
								    "Warning: detected poor recording quality. Code=0x%x (%s)",
							    (unsigned int) globalFeedback, checkFlagsText(globalFeedback));
						    DISP(cons, str);
					    }

					    user[uIdx].numEnroll += 1;
					    user[uIdx].enroll = realloc(user[uIdx].enroll,
							    sizeof(udtEnrollment_t) * user[uIdx].numEnroll);
					    user[uIdx].enroll[eIdx].pronun = NULL;
					    user[uIdx].enroll[eIdx].reserved1 = NULL;
					    user[uIdx].enroll[eIdx].audio = malloc(
							    sdetSpeechLen * sizeof(short));
					    memcpy(user[uIdx].enroll[eIdx].audio, sdetSpeech,
							    sdetSpeechLen * sizeof(short));
					    user[uIdx].enroll[eIdx].audioLen = sdetSpeechLen;
					    user[uIdx].enroll[eIdx].filename = malloc(
							    sizeof(char) * MAXSTR);
					    snprintf(user[uIdx].enroll[eIdx].filename, MAXSTR - 1,
							    "user%u_enroll%u.wav", uIdx + 1, eIdx + 1);

					    // Store pipelined pronunciation to save recomputing during enrollment
					    user[uIdx].enroll[eIdx].pronun = malloc(sizeof(char) * (strlen(rres)+1));
					    strcpy(user[uIdx].enroll[eIdx].pronun, rres);
    
  #if SAVE_RECORDINGS
					    if (sdetSpeech) {
						    DISP(cons, "Saving wave...");
						    DISP(cons, user[uIdx].enroll[eIdx].filename);
						    if (!thfWaveSaveToFile(ses,
								    PATH(0,user[uIdx].enroll[eIdx].filename),
								    (short *) sdetSpeech, sdetSpeechLen,
								    SAMPLERATE))
							    THROW("thfWaveSaveToFile");
					    }
  #endif

				    } else {
					    DISP(cons, "No recognition result");
				    }
			    } else {
				    DISP(cons, "End of speech not found");
			    }
			    thfRecogReset(ses, enrollRecog);
		    }
      }
		  /* check set of recordings */
		  DISP(cons, "Calculating quality...");
		  if (!thfUdtCheckEnrollments(ses, udt, user, uIdx, &globalFeedback,
				  feedbackArray, &phraseQuality)) {
			  THROW("thfUdtCheckEnrollments");
		  }
		  /* display feedback on set of recordings */
		  /* check feedback; if non-zero, display warning about particular recording(s) */
			
		  if(phraseQuality<0.35)
		    sprintf(str, "Phrase quality: %.2f (VERY POOR)\n", phraseQuality);
		  else if(phraseQuality<0.5)
		    sprintf(str, "Phrase quality: %.2f (POOR)\n", phraseQuality);
		  else if(phraseQuality<0.7)
		    sprintf(str, "Phrase quality: %.2f (FAIR)\n", phraseQuality);
		  else
		    sprintf(str, "Phrase quality: %.2f (GOOD)\n", phraseQuality);

		  DISP(cons, str);
		  if (globalFeedback != 0) {
			  for (eIdx = 0; eIdx < numEnroll; eIdx++) {
				  if (feedbackArray[eIdx] != 0) {
					  sprintf(str,
							  "Warning: poor recording detected for user %d, enrollment %d. Code=0x%x (%s)",
							  uIdx + 1, eIdx + 1,
						  (unsigned int) feedbackArray[eIdx], checkFlagsText(feedbackArray[eIdx]));
					  DISP(cons, str);
				  }
			  }
		  }
	  }

		DISP(cons, "\nLearning enrollments from all users...");
		/* --------- Learn enrollments for all users ------ */

		/* demonstrate how to set some parameter values; see documentation of thfUdtConfigSet() to see relevant parameters */
		if (!thfUdtConfigSet(ses, udt, UDT_PARAM_A_START, -1200))
			THROW("thfPhrasespotConfigSet");
		if (!thfUdtConfigSet(ses, udt, UDT_SAMP_PER_CAT, 512))
			THROW("thfSpeakerConfigSet");

		/* do enrollment */
		res = thfUdtEnroll(ses, udt, numUsers, user, &UDTsearch, &UDTrecog,
				NULL, NULL );
		if (res == 0) {
			THROW("thfUdtEnroll");
		}

		/* if search or recognizer is NULL, there's a problem */
		if (UDTsearch == NULL || UDTrecog == NULL ) {
			DISP(cons,
					"Error: thfUdtEnroll() returned 1, but UDTsearch or UDTrecog is NULL");
			THROW("thfUdtEnroll");
		}

		/* EPQ is turned on after calling thfUdtEnroll(), but user may want to turn it off */
#if TURN_EPQ_OFF
		if (!thfRecogConfigSet(ses,UDTrecog,REC_EPQ_ENABLE,0))
			THROW("thfRecogConfigGet");
#endif

		res = thfUdtSaveToFile(ses, UDTsearch, UDTrecog,
				PATH(0,TRIGGER_SEARCHFILE), PATH(1,TRIGGER_RECOGFILE));
		if (res == 0) {
			THROW("thfUdtSaveToFile");
		}

		// Optional: save recognizer and search in deeply embedded format
		if (!thfSaveEmbedded(ses, UDTrecog, UDTsearch, TARGET,
				PATH(0,EMBEDDED_RECOGFILE), PATH(1,EMBEDDED_SEARCHFILE), 0, 1))
			THROW("thfSaveEmbedded");

		// read in SID-specific recognizer and search from file
		if (!(recogTrigger = thfRecogCreateFromFile(ses,
				PATH(0,TRIGGER_RECOGFILE), 0xFFFF, -1, NO_SDET)))
			THROW("thfRecogCreateFromFile");
		if (!(searchTrigger = thfSearchCreateFromFile(ses, recogTrigger,
				PATH(0,TRIGGER_SEARCHFILE), NBEST)))
			THROW("thfSearchCreateFromFile");

    // save UDT enrollment data if enabled
    if (UDT_CACHE)  {
      if (!thfUdtCacheSaveToFile(ses, udt, PATH(0,UDT_CACHEFILE))) {
			  THROW("thfUdtCacheSaveToFile");
      }
    }

		// Delete enrollment data
		for (uIdx = 0; uIdx < numUsers; uIdx++) {
			if (user[uIdx].userName)
				free(user[uIdx].userName);
			user[uIdx].userName = NULL;
			for (eIdx = 0; eIdx < user[uIdx].numEnroll; eIdx++) {
				if (user[uIdx].enroll[eIdx].audio)
					free(user[uIdx].enroll[eIdx].audio);
				user[uIdx].enroll[eIdx].audio = NULL;
				if (user[uIdx].enroll[eIdx].filename)
					free(user[uIdx].enroll[eIdx].filename);
				user[uIdx].enroll[eIdx].filename = NULL;
				if (user[uIdx].enroll[eIdx].pronun)
					free(user[uIdx].enroll[eIdx].pronun);
				user[uIdx].enroll[eIdx].pronun = NULL;
			}
			if (user[uIdx].enroll != NULL )
				free(user[uIdx].enroll);
			user[uIdx].enroll = NULL;
		}
		free(user);
		user = NULL;
	}

	/* --------- Test phase ---------*/
	if (!thfPhrasespotConfigSet(ses,recogTrigger,searchTrigger,PS_DELAY,delay))
	  THROW("thfPhrasespotConfigSet: delay");
	if (!thfRecogInit(ses, recogTrigger, searchTrigger, RECOG_KEEP_WAVE))
	  THROW("thfRecogInit");

	DISP(cons, "Testing...");
	/* Record enroll phrases for each user */
	for (i = 0; i < NUM_TEST; i++) {
		sprintf(str, "\n===== Test %d of %d =====\n", i + 1, NUM_TEST);
		DISP(cons, str);

		/* Pipelined recognition */
		if (START_REC() == RECOG_NODATA)
			break;
		done = 0;
		rres = NULL;
		sprintf(str, "Say user-defined password");
		DISP(cons, str);
		while (!done) { /* Feeds audio into recognizer until sdet done */
			audioEventLoop();
			while ((aud = GET_NEXT_BUFFER()) && !done) {
				if (!thfRecogPipe(ses, recogTrigger, aud->len, aud->audio,
						RECOG_ONLY, &status))
					THROW("recogPipe");
				switch (status) {
				case RECOG_QUIET:
					DISP(cons, "quiet");
					break;
				case RECOG_SOUND:
					DISP(cons, "sound");
					break;
				case RECOG_IGNORE:
					DISP(cons, "ignored");
					break;
				case RECOG_SILENCE:
					DISP(cons, "timeout: silence");
					break;
				case RECOG_MAXREC:
					DISP(cons, "timeout: maxrecord");
					break;
				case RECOG_DONE:
					DISP(cons, "end-of-speech");
					break;
				default:
					DISP(cons, "other");
					break;
				}
				audioNukeBuffer(aud);
				aud = NULL;
				if (status == RECOG_DONE) {
					done = 1;
					STOP_REC();
				}
			}
			if (aud) audioNukeBuffer(aud);
		}

		if (status == RECOG_DONE) {
			float score = 0;
			float svScore;

			if (!thfRecogResult(ses, recogTrigger, &score, &rres, NULL, NULL,
					NULL, NULL, NULL, NULL ))
				THROW("thfRecogResult");
			if (rres) {
				sprintf(str, "Result: %s", rres);
				DISP(cons, str);

#if UDTSID  // UDT+SID mode		    
				svScore = thfRecogSVscore(ses, recogTrigger);
				sprintf(str, "SV score: %f", svScore);
				DISP(cons, str);

				if (svScore >= SV_THRESHOLD) {
					sprintf(str, "ACCEPTED %s\n", rres);
					DISP(cons, str);
				} else {
					sprintf(str, "REJECTED %s\n", rres);
					DISP(cons, str);
				}
#else  // UDT-only mode
				DISP(cons, "TRIGGER");
#endif
			} else {
				DISP(cons, "No recognition result");
			}
		}
		thfRecogReset(ses, recogTrigger);

	}

	/* Clean up */
	DISP(cons, "Cleaning up...");
	killAudio();
	if (feedbackArray)
		free(feedbackArray);
	feedbackArray = NULL;
	if (enrollRecog)
		thfRecogDestroy(enrollRecog);
	enrollRecog = NULL;
	if (enrollSearch)
		thfPhonemeSearchDestroy(enrollSearch);
	enrollSearch = NULL;
	if (recogTrigger)
		thfRecogDestroy(recogTrigger);
	recogTrigger = NULL;
	if (searchTrigger)
		thfSearchDestroy(searchTrigger);
	searchTrigger = NULL;
	if (UDTsearch)
		thfSearchDestroy(UDTsearch);
	UDTsearch = NULL;
	if (UDTrecog)
		thfRecogDestroy(UDTrecog);
	UDTrecog = NULL;
	if (udt)
		thfUdtDestroy(udt);
	udt=NULL;
	thfSessionDestroy(ses);
	ses = NULL;
	DISP(cons, "Done");
	closeConsole(cons);

	return 0;

	/* Async error handling, see console.h for panic */
	error:
	killAudio();
	if (!ewhat && ses)
		ewhat = thfGetLastError(ses);
	if (fp)
		fclose(fp);

	// Delete enrollment data
	if (user) {
		for (uIdx = 0; uIdx < numUsers; uIdx++) {
			if (user[uIdx].userName)
				free(user[uIdx].userName);
			for (eIdx = 0; eIdx < user[uIdx].numEnroll; eIdx++) {
				if (user[uIdx].enroll[eIdx].audio)
					free(user[uIdx].enroll[eIdx].audio);
				if (user[uIdx].enroll[eIdx].filename)
					free(user[uIdx].enroll[eIdx].filename);
				if (user[uIdx].enroll[eIdx].pronun)
					free(user[uIdx].enroll[eIdx].pronun);
			}
			if (user[uIdx].enroll != NULL )
				free(user[uIdx].enroll);
		}
		free(user);
		user = NULL;
	}
	if (feedbackArray)
		free(feedbackArray);
	feedbackArray = NULL;
	if (enrollRecog)
		thfRecogDestroy(enrollRecog);
	enrollRecog = NULL;
	if (enrollSearch)
		thfPhonemeSearchDestroy(enrollSearch);
	enrollSearch = NULL;
	if (recogTrigger)
		thfRecogDestroy(recogTrigger);
	recogTrigger = NULL;
	if (searchTrigger)
		thfSearchDestroy(searchTrigger);
	searchTrigger = NULL;
	if (UDTsearch)
		thfSearchDestroy(UDTsearch);
	UDTsearch = NULL;
	if (UDTrecog)
		thfRecogDestroy(UDTrecog);
	UDTrecog = NULL;
	if (udt)
		thfUdtDestroy(udt);
	udt=NULL;

	sprintf(str, "\nERROR: %s: %s", ewhere,ewhat); DISP(cons, str);
	if (ses)
		thfSessionDestroy(ses);
	return 1;
}
