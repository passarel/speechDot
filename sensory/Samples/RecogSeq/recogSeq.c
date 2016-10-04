/*
* Sensory Confidential
*
* Copyright (C)2000-2015 Sensory Inc.
*
*---------------------------------------------------------------------------
* Sensory TrulyNatural SDK Example: recogSeq.c
* Using a phrasespot search and a grammar search
* performs spotting and then recognition on live speech (... or simulated live speech).
*  - Uses live speech (or simulated live speech) as input.
*  - Uses pre-constructed phrasespot search and grammar search (e.g. built by Sensory).
*  - Illustrates pipelined recognition for phrasespotting and grammar recognition,
*    as well as sequential recognition (e.g. phrasespotting followed immediately by
*    grammar recognition)
*  - Illustrates how to calibrate speech detector, and recognizers
*    and searches for sequential recogntion.
*---------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <audio.h>
#include <datalocations.h>

// PhraseSpot 
#define PS_NET      "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_am.raw"
#define PS_SEARCH   "sensory_demo_hbg_en_us_sfs_delivery04_pruned_search_9.raw"

// RecogSong
#define GRAMMAR_NET    "nn_en_us_mfcc_16k_15_250_v5.1.1.raw"
#define GRAMMAR_LTS    "lts_en_us_9.5.2b.raw"
#define GRAMMAR_SEARCH "names.raw"

#define KEEP_FEATURES  0
#define SDET_TSILENCE  500
#define SDET_LSILENCE  5000

#define BEAM  200.0f
#define NBEST  3
#define PHRASESPOT_DELAY  90

/*
 * Command line for using recorded audio files as if they were live input.
 * Replace <username> with a value appropriate to your environment:
 *
env FSSDK_INPUT_FILES="<path>/Data/hbg_john_smith_16kHz.wav;<path>/Data/hbg_curtis_craft_16kHz.wav;<path>/Data/hbg_may_johnson_16kHz.wav" ./recogSeq
 */

#define STRSZ   (128)
#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a,b) { ewhere=(a); ewhat=(b); goto error; }
#define inst NULL

int main(int argc, char **argv)
{
  const char *rres;
  thf_t *ses = NULL;
  recog_t *r = NULL, *psr = NULL;
  searchs_t *s = NULL, *pss = NULL;
  char str[STRSZ];
  float score, delay, from, to, beam, garbage, any, nota;
  unsigned short ii, i, status, done = 0;
  audiol_t *p = NULL;
  FILE *fp = NULL;
  const char *ewhere, *ewhat = NULL;
  void *cons = NULL;

  /* Create SDK session */
  if (!(ses = thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

  /* Create phrasespot recognizer */
  disp(cons, "Loading phrasespot recognizer: " SAMPLES_DATADIR PS_NET);
  if (!(psr = thfRecogCreateFromFile(ses, SAMPLES_DATADIR PS_NET,
    0xfffff, 0xffff, NO_SDET)))
    THROW("recogCreateFromFile");

  /* Create phrasespot search */
  disp(cons, "Loading search: " SAMPLES_DATADIR PS_SEARCH);
  if (!(pss = thfSearchCreateFromFile(ses, psr, SAMPLES_DATADIR PS_SEARCH, 1)))
    THROW("searchCreateFromFile");


  /* Check if grammar search data file exists */
  if (!(fp = fopen(SAMPLES_DATADIR GRAMMAR_SEARCH, "rb"))) {
    panic(cons, "Missing data file: ", SAMPLES_DATADIR GRAMMAR_SEARCH, 0);
    return 1;
  }
  fclose(fp); fp = NULL;

  /* Create grammar recognizer */
  disp(cons, "Loading grammar recognizer: " THF_DATADIR GRAMMAR_NET);
  if (!(r = thfRecogCreateFromFile(ses, THF_DATADIR GRAMMAR_NET, 0xffff, 0xffff, SDET)))
    THROW("recogCreateFromFile");

  /* Create grammar search */
  disp(cons, "Loading search: " SAMPLES_DATADIR GRAMMAR_SEARCH);
  if(!(s=thfSearchCreateFromFile(ses,r,SAMPLES_DATADIR GRAMMAR_SEARCH,NBEST))) 
    THROW("searchCreateFromFile");

  /* Initialize grammar recognizer */
  disp(cons, "Initializing grammar recognizer...");
  if (!thfRecogInit(ses, r, s, RECOG_KEEP_WAVE_WORD_PHONEME)) THROW("thfRecogInit");

  /* Configure grammar Recognition (REC) parameters (optional) */
  if (!thfRecogConfigSet(ses, r, REC_KEEP_FEATURES, KEEP_FEATURES))
    THROW("thfRecogConfigSet: KEEP_FEATURES");
  if (!thfRecogConfigSet(ses, r, REC_LSILENCE, SDET_LSILENCE))
    THROW("thfRecogConfigSet: TSILENCE");
  if (!thfRecogConfigSet(ses, r, REC_TSILENCE, SDET_TSILENCE))
    THROW("thfRecogConfigSet: TSILENCE");

  beam=BEAM; garbage=0.5; any=0.5; nota=0.5f;
  if (!thfSearchConfigSet(ses,r,s,SCH_BEAM,beam))
    THROW("searchConfigSet: beam");
  if (!thfSearchConfigSet(ses,r,s,SCH_GARBAGE,garbage))
    THROW("searchConfigSet: garbage");
  if (!thfSearchConfigSet(ses,r,s,SCH_ANY,any))
    THROW("searchConfigSet: any");
  if (!thfSearchConfigSet(ses,r,s,SCH_NOTA,nota))
    THROW("searchConfigSet: nota");

  /* Initialize phrasespot recognizer */
  disp(cons, "Initializing phrasespot recognizer...");
  if (!thfRecogInit(ses, psr, pss, RECOG_KEEP_WAVE_WORD)) THROW("thfRecogInit");

  /* Configure phrasespot parameters - only DELAY... others are saved in search already */
  disp(cons, "Setting phrasespot parameters...");
  delay = PHRASESPOT_DELAY;
  if (!thfPhrasespotConfigSet(ses, psr, pss, PS_DELAY, delay))
    THROW("thfPhrasespotConfigSet: delay");

  // One second sequential buffer
  if (!thfPhrasespotConfigSet(ses, psr, pss, PS_SEQ_BUFFER, 1000))
    THROW("thfPhrasespotConfigSet: ps_seq_buffer");

  /* Initialize audio */
  disp(cons, "Initializing audio...");
  initAudio(inst, cons);

  for (ii = 0; ii<3; ii++) {
    sprintf(str, "\n\n========== Loop %i of 3 ==========\n",ii + 1); disp(cons, str);
    if (startRecord() == RECOG_NODATA)
      break;
    disp(cons, "Vocabulary: <trigger>, <command>");
    disp(cons, "Example: Hello Blue Genie, John Smith");
    disp(cons, ">>> SPEAK NOW <<<");

    /* Pipelined recognition */
    done = 0;
    while (!done) {
      audioEventLoop();
      while ((p = audioGetNextBuffer(&done)) && !done) {
        if (!thfRecogPipe(ses, psr, p->len, p->audio, RECOG_ONLY, &status))
          THROW("recogPipe");
        audioNukeBuffer(p);p = NULL;
        if (status == RECOG_DONE) {
          done = 1;
        }
      }
    }

    /* Report N-best recognition result */
    rres = NULL;
    switch (status) {
      case RECOG_QUIET:   disp(cons, "quiet"); break;
      case RECOG_SOUND:   disp(cons, "sound"); break;
      case RECOG_IGNORE:  disp(cons, "ignored"); break;
      case RECOG_SILENCE: disp(cons, "timeout: silence"); break;
      case RECOG_MAXREC:  disp(cons, "timeout: maxrecord"); break;
      case RECOG_DONE:    disp(cons, "end-of-speech"); break;
      default:            disp(cons, "other"); break;
    }
    if (status == RECOG_DONE) {
      const char *walign, *palign;
      if (!thfRecogResult(ses, psr, &score, &rres, &walign, &palign, NULL, NULL, NULL, NULL))
        THROW("thfRecogResult");
      if (rres == 0) break;
      sprintf(str, "Phrasespot Result: %s (%ld)\n%s", rres, (long)(1000 * score + .5), walign); 
      disp(cons, str);
    }
    else
      disp(cons, "No phrasespot recognition result");

    if (!thfRecogPrepSeq(ses, r, psr))
      THROW("thfRecogPrepSeq");
    if (!thfRecogReset(ses, psr))
      THROW("thfRecogReset");

    /* Pipelined recognition */
    done = 0;
    while (!done) {
      audioEventLoop();
      while ((p = audioGetNextBuffer(&done)) && !done) {
        if (!thfRecogPipe(ses, r, p->len, p->audio, SDET_RECOG, &status))
          THROW("recogPipe");
        audioNukeBuffer(p);p = NULL;

        if (status != RECOG_QUIET && status != RECOG_SOUND) {
          done = 1;
          stopRecord();
          disp(cons, "Stopped recording");
        }
      }
    }

    /* Report N-best recognition result */
    rres = NULL;
    if (status == RECOG_DONE || status == RECOG_MAXREC) {
      thfRecogGetSpeechRange(ses, r, &from, &to);
      sprintf(str, "Found speech from %ld to %ld ms", (long)from, (long)to); disp(cons, str);
      disp(cons, "Recognition results...");
      {
        float clip;
        if (!thfRecogGetClipping(ses, r, &clip)) THROW("thfRecogGetClipping");
        if (clip>0.0) disp(cons, "WARNING: Speech is clipped");
      }
      score = 0;
      for (i = 0; i<NBEST; i++) {
        const char *walign = NULL, *palign = NULL;
        unsigned long inSpeechSamples = 0, sdetSpeechSamples = 0;
        const short *inSpeech = NULL, *sdetSpeech = NULL;

        if (!thfRecogResult(ses, r, &score, &rres, &walign, &palign, &inSpeech,
          &inSpeechSamples, &sdetSpeech, &sdetSpeechSamples))
          THROW("thfRecogResult");
        if (rres == 0) break;
        sprintf(str, "Result %i: %s (%ld)", i + 1, rres, (long)(1000 * score + .5)); disp(cons, str);
      }
    }
    else
      disp(cons, "No recognition result");
    thfRecogReset(ses, r);
  }

  /* Clean up */
  killAudio();
  if (p) audioNukeBuffer(p);
  thfRecogDestroy(psr);
  thfSearchDestroy(pss);
  thfRecogDestroy(r);
  thfSearchDestroy(s);
  thfSessionDestroy(ses);
  disp(cons, "Done");
  closeConsole(cons);
  return 0;

  /* Async error handling, see console.h for panic */
error:
  killAudio();
  if (p) audioNukeBuffer(p);
  if (!ewhat && ses) ewhat = thfGetLastError(ses);
  if (fp) fclose(fp);
  if (psr) thfRecogDestroy(psr);
  if (pss) thfSearchDestroy(pss);
  if (r) thfRecogDestroy(r);
  if (s) thfSearchDestroy(s);
  panic(cons, ewhere, ewhat, 0);
  if (ses) thfSessionDestroy(ses);
  return 1;
}
