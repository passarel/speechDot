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

#define BUILD_MODE        (MODE_BUILD_AUTO)

#define GENDER_DETECT (1)  // Enable/disable gender detection

//#define HBG_GENDER_MODEL "hbg_genderModel.raw"
#define HBG_GENDER_MODEL "/home/pi/speakspoke/sensory/Samples/Data/hbg_genderModel.raw"

//#define HBG_NETFILE      "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_am.raw"
#define HBG_NETFILE      "/home/pi/speakspoke/sensory/Samples/Data/sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_am.raw"

//#define HBG_SEARCHFILE   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_9.raw" // pre-built search
#define HBG_SEARCHFILE   "/home/pi/speakspoke/sensory/Samples/Data/sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_9.raw" // pre-built search

#define HBG_OUTFILE      "myhbg.raw"           /* hbg output search */

#define NBEST              (1)                /* Number of results */
#define PHRASESPOT_PARAMA_OFFSET   (0)        /* Phrasespotting ParamA Offset */
#define PHRASESPOT_BEAM    (100.f)            /* Pruning beam */
#define PHRASESPOT_ABSBEAM (100.f)            /* Pruning absolute beam */

#define PHRASESPOT_DELAY  90                 /* Phrasespotting Delay */

#define MAXSTR             (512)              /* Output string size */
#define SHOWAMP            (0)                /* Display amplitude */

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) {ewhere=(a); ewhat=(b); goto error; }

#define KEEP_FEATURES  0
#define SDET_TSILENCE  500
#define SDET_LSILENCE  5000
#define BEAM  200.0f

char *formatExpirationDate(time_t expiration) {
	static char expdate[33];
	if (!expiration)
		return "never";
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
	thf_t *ses = NULL;

	recog_t *r_trig = NULL;
	pronuns_t *pp_trig = NULL;
	searchs_t *s_trig = NULL;
	const char *trigs[] = { "Ok Google", "Hey Seary", "Jessina", "Hey Kor tana", "Alexa" };
	unsigned short trigCount = sizeof(trigs) / sizeof(trigs[0]);

	recog_t *r_cmd = NULL;
	pronuns_t *pp_cmd = NULL;
	searchs_t *s_cmd = NULL;
	unsigned short cmdCount = 6;
	const char *cmds[] = { "Shutdown", "Reset", "Pair", "Answer", "Dismiss", "Hang Up" };

	char str[MAXSTR];
	const char *ewhere, *ewhat = NULL;
	float score, delay, from, to, garbage, any, nota;
	unsigned short status, done = 0;
	audiol_t *p;
	unsigned short i = 0;
	void *cons = NULL;

	float paramAOffset, beam, absbeam;

	// Draw console
	if (!(cons = initConsole(inst)))
		return 1;

	// Create SDK session
	if (!(ses = thfSessionCreate())) {
		panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
		return 1;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------
	// Create recognizers
	disp(cons, "Loading trig recognizer: " NETFILE);
	if (!(r_trig = thfRecogCreateFromFile(ses, THF_DATADIR NETFILE, 0xffff, 0xffff, NO_SDET)))
		THROW("thfRecogCreateFromFile");

	disp(cons, "Loading cmd recognizer: " NETFILE);
	if (!(r_cmd = thfRecogCreateFromFile(ses, THF_DATADIR NETFILE, 0xffff, 0xffff, SDET)))
		THROW("thfRecogCreateFromFile");
	//----------------------------------------------------------------------------------------------------------------------------------------


	//----------------------------------------------------------------------------------------------------------------------------------------
	// Create pronunciations
	disp(cons, "Loading pronunciation model: " LTSFILE);
	if (!(pp_trig = thfPronunCreateFromFile(ses, THF_DATADIR LTSFILE, 0)))
		THROW("thfPronunCreateFromFile");

	disp(cons, "Loading pronunciation model: " LTSFILE);
	if (!(pp_cmd = thfPronunCreateFromFile(ses, THF_DATADIR LTSFILE, 0)))
		THROW("thfPronunCreateFromFile");
	//----------------------------------------------------------------------------------------------------------------------------------------


	//----------------------------------------------------------------------------------------------------------------------------------------
	// Create searches
	disp(cons, "Compiling trig search...");
	if (!(s_trig = thfPhrasespotCreateFromList(ses, r_trig, pp_trig, (const char**) trigs, trigCount, NULL, NULL, 0, PHRASESPOT_LISTEN_CONTINUOUS)))
		THROW("thfPhrasespotCreateFromList");

	disp(cons, "Compiling cmd search...");
	if(!(s_cmd=thfSearchCreateFromList(ses, r_cmd, pp_cmd, (const char**) cmds, NULL, cmdCount, 1)))
		THROW("thfSearchCreateFromList");
	//----------------------------------------------------------------------------------------------------------------------------------------


	//----------------------------------------------------------------------------------------------------------------------------------------
	// Configure parameters
	disp(cons, "Setting trig parameters...");
	paramAOffset = PHRASESPOT_PARAMA_OFFSET;
	beam = PHRASESPOT_BEAM;
	absbeam = PHRASESPOT_ABSBEAM;
	delay = PHRASESPOT_DELAY;
	if (!thfPhrasespotConfigSet(ses, r_trig, s_trig, PS_PARAMA_OFFSET, paramAOffset))
		THROW("thfPhrasespotConfigSet: parama_offset");
	if (!thfPhrasespotConfigSet(ses, r_trig, s_trig, PS_BEAM, beam))
		THROW("thfPhrasespotConfigSet: beam");
	if (!thfPhrasespotConfigSet(ses, r_trig, s_trig, PS_ABSBEAM, absbeam))
		THROW("thfPhrasespotConfigSet: absbeam");
	if (!thfPhrasespotConfigSet(ses, r_trig, s_trig, PS_DELAY, delay))
		THROW("thfPhrasespotConfigSet: delay");
	if (!thfPhrasespotConfigSet(ses, r_trig, s_trig, PS_SEQ_BUFFER, 1000))
		THROW("thfPhrasespotConfigSet: ps_seq_buffer");

	disp(cons, "Setting cmd parameters...");
	beam = BEAM;
	garbage = 0.5;
	any = 0.5;
	nota = 0.5f;
	if (!thfSearchConfigSet(ses, r_cmd, s_cmd, SCH_BEAM, beam))
		THROW("searchConfigSet: beam");
	if (!thfSearchConfigSet(ses, r_cmd, s_cmd, SCH_GARBAGE, garbage))
		THROW("searchConfigSet: garbage");
	if (!thfSearchConfigSet(ses, r_cmd, s_cmd, SCH_ANY, any))
		THROW("searchConfigSet: any");
	if (!thfSearchConfigSet(ses, r_cmd, s_cmd, SCH_NOTA, nota))
		THROW("searchConfigSet: nota");

	  if (!thfRecogConfigSet(ses, r_cmd, REC_KEEP_FEATURES, KEEP_FEATURES))
	    THROW("thfRecogConfigSet: KEEP_FEATURES");
	  if (!thfRecogConfigSet(ses, r_cmd, REC_LSILENCE, SDET_LSILENCE))
	    THROW("thfRecogConfigSet: TSILENCE");
	  if (!thfRecogConfigSet(ses, r_cmd, REC_TSILENCE, SDET_TSILENCE))
	    THROW("thfRecogConfigSet: TSILENCE");
	//----------------------------------------------------------------------------------------------------------------------------------------


	// Initialize trigger recognizer
	disp(cons, "Initializing trigger recognizer...");
	if (!thfRecogInit(ses, r_trig, s_trig, RECOG_KEEP_WAVE_WORD)) THROW("thfRecogInit");

	// Initialize cmd recognizer
	disp(cons, "Initializing cmd recognizer...");
	if (!thfRecogInit(ses, r_cmd, s_cmd, RECOG_KEEP_WAVE_WORD_PHONEME)) THROW("thfRecogInit");

	// Initialize audio
	disp(cons, "Initializing audio...");
	initAudio(inst, cons);

	for (i = 1;; i++) {
		if (startRecord() == RECOG_NODATA)
			break;

		if (!thfRecogReset(ses, r_trig))
			THROW("thfRecogReset");

		/* Pipelined recognition */
		done = 0;
		while (!done) {
			audioEventLoop();
			while ((p = audioGetNextBuffer(&done)) && !done) {
				if (!thfRecogPipe(ses, r_trig, p->len, p->audio, RECOG_ONLY, &status)) {
					panic(cons, "thfRecogPipe", thfGetLastError(ses), 0);
					return 1;
				}
				audioNukeBuffer(p);
				p = NULL;
				if (status == RECOG_DONE) {
					done = 1;
				}
			}
		}

		/* Report N-best recognition result */
		rres = NULL;

		/*
		switch (status) {
		case RECOG_QUIET:
			disp(cons, "quiet");
			break;
		case RECOG_SOUND:
			disp(cons, "sound");
			break;
		case RECOG_IGNORE:
			disp(cons, "ignored");
			break;
		case RECOG_SILENCE:
			disp(cons, "timeout: silence");
			break;
		case RECOG_MAXREC:
			disp(cons, "timeout: maxrecord");
			break;
		case RECOG_DONE:
			disp(cons, "end-of-speech");
			break;
		default:
			disp(cons, "other");
			break;
		}
		*/

		if (status == RECOG_DONE) {
			const char *walign, *palign;
			if (!thfRecogResult(ses, r_trig, &score, &rres, &walign, &palign, NULL, NULL, NULL, NULL))
				THROW("thfRecogResult");
			if (rres == 0)
				break;
			//sprintf(str, "Phrasespot Result: %s (%ld)\n%s", rres, (long) (1000 * score + .5), walign);
			//disp(cons, rres);
			sprintf(str, "%s", rres);
		} else {
			disp(cons, "No phrasespot recognition result");
			continue;
		}

		if (strcmp(str, "Hey Seary") == 0 || strcmp(str, "Ok Google") == 0 || strcmp(str, "Hey Kor tana") == 0 ||
			strcmp(str, "Alexa") == 0) {
			disp(cons, str);
		    continue;
		}

		if (!thfRecogPrepSeq(ses, r_cmd, r_trig))
			THROW("thfRecogPrepSeq");

		//if (!thfRecogReset(ses, r_trig))
			//THROW("thfRecogReset");

		done = 0;
		while (!done) {
			audioEventLoop();
			while ((p = audioGetNextBuffer(&done)) && !done) {
				if (!thfRecogPipe(ses, r_cmd, p->len, p->audio, SDET_RECOG, &status)) {
					panic(cons, "thfRecogPipe", thfGetLastError(ses), 0);
					return 1;
				}
				audioNukeBuffer(p);
				p = NULL;
				if (status != RECOG_QUIET && status != RECOG_SOUND) {
					done = 1;
					stopRecord();
					//disp(cons, "Stopped recording");
				}
			}
		}

		rres = NULL;
		if (status == RECOG_DONE || status == RECOG_MAXREC) {
			thfRecogGetSpeechRange(ses, r_cmd, &from, &to);
			//sprintf(str, "Found speech from %ld to %ld ms", (long) from, (long) to);
			//disp(cons, str);
			//disp(cons, "Recognition results...");
			{
				float clip;
				if (!thfRecogGetClipping(ses, r_cmd, &clip))
					THROW("thfRecogGetClipping");
				if (clip > 0.0)
					disp(cons, "WARNING: Speech is clipped");
			}
			score = 0;
			for (i = 0; i < NBEST; i++) {
				const char *walign = NULL, *palign = NULL;
				unsigned long inSpeechSamples = 0, sdetSpeechSamples = 0;
				const short *inSpeech = NULL, *sdetSpeech = NULL;

				if (!thfRecogResult(ses, r_cmd, &score, &rres, &walign, &palign, &inSpeech, &inSpeechSamples, &sdetSpeech, &sdetSpeechSamples))
					THROW("thfRecogResult");
				if (rres == 0)
					break;
				//sprintf(str, "Result %i: %s (%ld)", i + 1, rres, (long) (1000 * score + .5));
				//disp(cons, str);
				sprintf(str, "%s %s", str, rres);
				disp(cons, str);
			}
		} else
			disp(cons, "No recognition result");
		thfRecogReset(ses, r_cmd);

	}

	inline void cleanup() {
		killAudio();
		if (r_trig)
			thfRecogDestroy(r_trig);
		r_trig = NULL;
		if (s_trig)
			thfSearchDestroy(s_trig);
		s_trig = NULL;
		if (pp_trig)
			thfPronunDestroy(pp_trig);
		pp_trig = NULL;
		if (ses)
			thfSessionDestroy(ses);
		ses = NULL;
		disp(cons, "Done");
	}

	// Clean up
	cleanup();
	closeConsole(cons);
	return 0;

	// Async error handling, see console.h for panic
	error:
	panic(cons,ewhere,ewhat,0);
	cleanup();
	return 1;
}
