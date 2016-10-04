/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *-----------------------------------------------------------------
 * Sensory TrulyHandsfree SDK: audio.c
 * Provides support for asynchronous audio input.
 * NOTE: for a real application, consider running audio I/O in a separate 
 * thread to avoid real time issues. E.g., failure to service the audio 
 * device in a timely manner could cause internal buffer over-run.
 *-----------------------------------------------------------------
 */

#include <trulyhandsfree.h>

#include <console.h>
#include <audio.h>

#if defined(_WINDOWS) || defined(_WIN32_) || defined(_WIN32)
/*-----------------------------------------------------------------*
  "win32" audio 
 *-----------------------------------------------------------------*/
#ifdef _WIN32_WCE
# define NAME _T("Audio")
#else
# define NAME "Audio"
#endif
#include <stdio.h>

#include <console.h>
#include <audio.h>

typedef struct {
  HWND hwnd;
  HINSTANCE inst;
  WAVEFORMATEX f;
  HWAVEIN hwi;
  HWAVEOUT hwo;
  WAVEHDR *buf[AUDIO_BUFFERS];
  int recording;
  audiol_t *last;
  void *cons;

  unsigned short bUseFileInput;
  char *fssdk_input_file_string;
  char **fssdk_input_file;
  unsigned short numFiles;
  unsigned long sampleIndex;
  unsigned short fileIndex;
  short *waveFileBuffer;
  unsigned long inlen;
  unsigned long sampleRate;
  thf_t *ses;
} audio_t;

#if !defined(UNICODE)
# define L(a) a
#endif

//#if !defined(_ONE_FILE_PER_INPUT_DEVICE_INSTANCE)
//# define _ONE_FILE_PER_INPUT_DEVICE_INSTANCE
//#endif

static audio_t aud;

static int audioPanic(char *why, MMRESULT r)
{
  char err[512];
#if defined(_UNICODE)
  wchar_t werr[512];
  waveInGetErrorText(r,werr,512);
  WideCharToMultiByte(CP_ACP,0,werr,-1,err,512,NULL,NULL);
#else
  waveInGetErrorText(r,err,512);
#endif
  panic(aud.cons,err,why,1);
  return 0;
}


static void audioDataEvent(HWAVEIN hwi, WAVEHDR *h)
{
  MMRESULT r=waveInUnprepareHeader(hwi,h,sizeof(WAVEHDR));
  if(aud.recording>=0) {
    int keep=0;
    if(r!=MMSYSERR_NOERROR) audioPanic("Unpreparing buffer",r);
    switch (aud.recording) {
    case 0:  
      break;
    case 1:  
      if(h->dwBytesRecorded) keep=1;
      break;
    default: /* discard after reset */ 
      if(aud.last && h->dwBytesRecorded) keep=1;
      aud.recording--; 
      break;
    }
    if(keep) { /* We are currently recording, so keep this */
      audiol_t *p=aud.last;
      unsigned i, j;
      /* lets work with aud.last */
      if(p) {
          while(p->n) p=p->n;
          p->n=memset(malloc(sizeof(audiol_t)),0,sizeof(audiol_t));
          p=p->n;
      } else
          p=aud.last=memset(malloc(sizeof(audiol_t)),0,sizeof(audiol_t));
      /* we assume that the buffer is smaller than an short in size */
      p->len=(unsigned short)
          (h->dwBytesRecorded/aud.f.nChannels/sizeof(short));
      p->audio=malloc(p->len*sizeof(short));
      for(i=j=0; i<p->len; i++, j+=aud.f.nChannels)
          p->audio[i]=((short *)h->lpData)[j];
      p->flags=0;
    }
    waveInPrepareHeader(hwi,h,sizeof(WAVEHDR));
    waveInAddBuffer(hwi,h,sizeof(WAVEHDR));
  } else aud.recording--;
} 

static LRESULT CALLBACK audioWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, 
				LPARAM lParam)
{
  switch (uMsg) {
  case MM_WIM_DATA:  
    audioDataEvent((HWAVEIN)wParam,(WAVEHDR *)lParam); 
    break;
  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}


void audioEventLoop() 
{ 
  MSG msg; 
  if (!aud.bUseFileInput)
  {
      if (PeekMessage(&msg, aud.hwnd, 0, 0, PM_REMOVE)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
      }
  }
} 


int startRecord()
{
  if (!aud.bUseFileInput) {
    /* first wipe old recording */
    while (aud.last) {
      audiol_t *p = aud.last;
      aud.last = aud.last->n;
      if (p->audio) free(p->audio);
      free(p);
    }
    aud.last = NULL;
    /* here we must allocate the buffers */
    if (aud.hwi) {
      aud.recording = 3;
      return 1;
    }
    return 0;
  }
  else {
    /* aud.bUseFileInput == true */
    if (aud.sampleIndex == 0)
    {
      while (aud.fileIndex < aud.numFiles) {
        char str[512];
        sprintf(str, "FILE=%s\n", aud.fssdk_input_file[aud.fileIndex]);
        disp(aud.cons, str);

        if (!(thfWaveFromFile(aud.ses, aud.fssdk_input_file[aud.fileIndex++], &aud.waveFileBuffer,
          &aud.inlen, &aud.sampleRate)))
        {
          sprintf(str, "thfWaveFromFile failed for %s", aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          killAudio();
          exit(1);
        }
        if (aud.sampleRate != SAMPLERATE) {
          sprintf(str, "Warning: Input sampleRate(%f) differs from defined sampleRate(%f). Skipping file (%s).", 
            SAMPLERATE, aud.sampleRate,aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          continue;
        }
        return 0;
      }
      if (aud.fileIndex >= aud.numFiles) {
        return RECOG_NODATA;
      }
    }
    else
      return 0;
  }
  return RECOG_NODATA;
}

int stopRecord()
{
  if (!aud.bUseFileInput)
  {
    if (aud.hwi) {
      MMRESULT r;
      aud.recording = AUDIO_BUFFERS + 1;
      if ((r = waveInReset(aud.hwi)) != MMSYSERR_NOERROR)
        return audioPanic("Could not reset recording", r);
      while (aud.recording > 1 &&
        MsgWaitForMultipleObjectsEx(0, NULL, 10, QS_ALLEVENTS, 0))
        audioEventLoop();
      aud.recording = 0;
      if ((r = waveInStart(aud.hwi)) != MMSYSERR_NOERROR)
        return audioPanic("Could not restart recording", r);
      return 1;
    }
  }
  else
  {
    if (aud.waveFileBuffer != NULL)
    {
      thfFree(aud.waveFileBuffer);
      aud.waveFileBuffer = NULL;
    }
    aud.sampleIndex = 0;
    aud.inlen = 0;
    aud.sampleRate = 0;
  }
  return 0;
}


static char** get_env_values(char* input, unsigned short *size)
{
  // The limit is VLimit number of individual input files
  size_t VLimit = 500;
  char seps[] = ";";
  char** retVals = NULL;
  const char* val;
  char* nextVal = NULL;

  *size = 0;
  if (!input)
    return NULL;

  retVals = (char**)malloc(sizeof(char*)*VLimit);

  val = strtok_s(input, seps, &nextVal);
  while (val != NULL)
  {
    if ((*size) >= VLimit)
      break; // Input vals list exceeds allocated memory.

    // Trim leading space
    while (isspace(*val)) val++;
    if (*val != 0)  // All spaces?
    {
      // Trim trailing space
      char* end = (char*)(val + strlen(val) - 1);
      while (end > val && isspace(*end)) end--;
      end++;
      end[0] = 0;

      retVals[(*size)++] = (char*)val;
    }
    val = strtok_s(NULL, seps, &nextVal);
  }
  return retVals;
}

int initAudio_ex(HINSTANCE inst, void *c, unsigned short *file_input)
{
  int r = initAudio(inst, c);
  *file_input = aud.bUseFileInput;
  return r;
}

/*
 * Records all the time due to slow startup of some audio hardware
 */
int initAudio(HINSTANCE inst, void *c)
{
  char* pEnvFssdkInputFileString = NULL;

  MMRESULT r, i;
  WNDCLASS wc;
  aud.inst = inst;
  
  aud.bUseFileInput = 0;
  aud.numFiles = 0;
  aud.sampleIndex = 0;
  aud.fileIndex = 0;
  aud.waveFileBuffer = NULL;
  aud.inlen = 0;
  aud.sampleRate = 0;
  aud.ses = NULL;

  pEnvFssdkInputFileString = getenv("FSSDK_INPUT_FILES");
  if (pEnvFssdkInputFileString)
  {
    // getenv returns a pointer to the environment table entry containing FSSDK_INPUT_FILES.
    // It is not safe to modify the value of the environment variable using the returned pointer.
    // So copy it to a local static location.
    aud.fssdk_input_file_string = strcpy(malloc(strlen(pEnvFssdkInputFileString) + 1), pEnvFssdkInputFileString);
    aud.fssdk_input_file = get_env_values(aud.fssdk_input_file_string, &aud.numFiles);

    if (aud.fssdk_input_file != NULL && aud.numFiles > 0)
    {
      aud.bUseFileInput = 1;

      /* Create SDK session for use with thfGetWaveFromFile */
      if (!(aud.ses = thfSessionCreate())) {
        panic(aud.cons, "thfSessionCreate", thfGetLastError(NULL), 0);
        return 1;
      }
    }
  }

  /* Create window */
  memset(&wc,0,sizeof(wc));
  wc.style = 0;
  wc.lpfnWndProc = (WNDPROC)audioWndProc;
  wc.hInstance = aud.inst;
  
  wc.lpszClassName = (LPCTSTR)NAME;
  if(!RegisterClass(&wc)) return FALSE;
  aud.hwnd = NULL;
  if(!aud.bUseFileInput)
    if (!(aud.hwnd = CreateWindowEx(0, (LPCTSTR)NAME, (LPCTSTR)NAME, 0, 0, 0,
         0, 0, NULL, NULL, aud.inst, NULL)));
  aud.f.wFormatTag=WAVE_FORMAT_PCM;
  aud.f.wBitsPerSample=16;
  aud.f.nChannels = 1;
  if (!aud.bUseFileInput)
    aud.f.nChannels = 2;
  aud.f.nSamplesPerSec=SAMPLERATE;
  aud.f.nBlockAlign=aud.f.nChannels*aud.f.wBitsPerSample/8;
  aud.f.nAvgBytesPerSec=aud.f.nBlockAlign*aud.f.nSamplesPerSec;
  aud.f.cbSize=0;
  aud.hwo=NULL;
  aud.cons=c;

  if (!aud.bUseFileInput)
  {
      if ((r = waveInOpen(&aud.hwi, WAVE_MAPPER, &aud.f, (DWORD)aud.hwnd, 42,
            CALLBACK_WINDOW)) != MMSYSERR_NOERROR)
          return audioPanic("Opening microphone", r);
      aud.recording=0;
      for (i = 0; i < AUDIO_BUFFERS; i++) {
          aud.buf[i] = memset(malloc(sizeof(WAVEHDR)), 0, sizeof(WAVEHDR));
          aud.buf[i]->dwBufferLength =
              SAMPLERATE*AUDIO_BUFFERSZ*sizeof(short)*aud.f.nChannels / 1000;
          aud.buf[i]->lpData = malloc(aud.buf[i]->dwBufferLength);
          if ((r = waveInPrepareHeader(aud.hwi, aud.buf[i], sizeof(WAVEHDR)))
                != MMSYSERR_NOERROR)
              return audioPanic("Preparing recording buffer", r);
          if ((r = waveInAddBuffer(aud.hwi, aud.buf[i], sizeof(WAVEHDR)))
                != MMSYSERR_NOERROR)
              return audioPanic("Adding recording buffer", r);
      }
  }

  if (!aud.bUseFileInput)
  {
      if ((r = waveInStart(aud.hwi)) != MMSYSERR_NOERROR)
          return audioPanic("Could not start recording", r);
  }
  return 1;
}

void killAudio()
{
  int i;
  if(aud.hwi) {
    waveInStop(aud.hwi);
    waveInReset(aud.hwi);
    waveInClose(aud.hwi);
  }
  if(aud.hwo) {
    waveOutReset(aud.hwo);
    waveOutClose(aud.hwo);
  }

  while (aud.last) {
    audiol_t *p = aud.last;
    aud.last = aud.last->n;
    if (p->audio) free(p->audio);
    free(p);
  }
  aud.last = NULL;

  for(i=0; i<AUDIO_BUFFERS; i++)
    if(aud.buf[i]) {
      if(aud.buf[i]->lpData) free(aud.buf[i]->lpData);
      free(aud.buf[i]);
    }
  DestroyWindow(aud.hwnd);
 
  if (aud.fssdk_input_file != NULL) {
    free(aud.fssdk_input_file);
    aud.fssdk_input_file = NULL;
    aud.numFiles = 0;
  }
  if (aud.fssdk_input_file_string != NULL) {
    free(aud.fssdk_input_file_string);
    aud.fssdk_input_file_string = NULL;
  }

  if (aud.ses) {
    thfSessionDestroy(aud.ses);
    aud.ses = NULL;
  }

  UnregisterClass((LPCTSTR)NAME, aud.inst);
}


#elif defined(__linux__)

/* Choose between ALSA or OSS interface in audio.h*/
#include  <unistd.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <console.h>
#include  <audio.h>
#if defined(USE_ALSA)
# include  <alsa/asoundlib.h>
#elif defined(USE_OSS)
# include  <sys/ioctl.h>
# include  <fcntl.h>
# include  <sys/soundcard.h>
#endif
 
# undef  strtok_s
# define strtok_s(a,b,c) strtok((a),(b))

extern char **environ;

typedef struct {  
  int audio;
  int recording;
  audiol_t *last;
  void *cons;
#ifdef USE_ALSA
  snd_pcm_t *inhandle;
  snd_pcm_t *outhandle;
#endif
  unsigned short bUseFileInput;
  char *fssdk_input_file_string;
  char **fssdk_input_file;
  unsigned short numFiles;
  unsigned long sampleIndex;
  unsigned short fileIndex;
  short *waveFileBuffer;
  unsigned long inlen;
  unsigned long sampleRate;
  thf_t *ses;
} audio_t;
static audio_t aud;


#ifdef USE_ALSA
/*-----------------------------------------------------------------*
  "ALSA" audio
 *-----------------------------------------------------------------*/

#define ALSA_PCM_NEW_HW_PARAMS_API  /* Use the newer ALSA API */

/* Display information about current PCM interface */
void aboutAlsa(snd_pcm_t *handle,snd_pcm_hw_params_t *params) {
  unsigned int val, val2;
  int val0;
  int dir;
  snd_pcm_uframes_t frames;

  printf("ALSA library version: %s\n",SND_LIB_VERSION_STR);
  printf("PCM handle name = '%s'\n",snd_pcm_name(handle));
  printf("PCM state = %s\n",snd_pcm_state_name(snd_pcm_state(handle)));
  snd_pcm_hw_params_get_access(params,(snd_pcm_access_t *) &val);
  printf("access type = %s\n",snd_pcm_access_name((snd_pcm_access_t)val));
  snd_pcm_hw_params_get_format(params, &val0);
  printf("format = '%s' (%s)\n",snd_pcm_format_name((snd_pcm_format_t)val0),
    snd_pcm_format_description((snd_pcm_format_t)val0));
  snd_pcm_hw_params_get_subformat(params,(snd_pcm_subformat_t *)&val);
  printf("subformat = '%s' (%s)\n",snd_pcm_subformat_name((snd_pcm_subformat_t)val),
    snd_pcm_subformat_description((snd_pcm_subformat_t)val));
  snd_pcm_hw_params_get_channels(params, &val);
  printf("channels = %d\n", val);
  snd_pcm_hw_params_get_rate(params, &val, &dir);
  printf("rate = %d bps\n", val);
  snd_pcm_hw_params_get_period_time(params,&val, &dir);
  printf("period time = %d us\n", val);
  snd_pcm_hw_params_get_period_size(params,&frames, &dir);
  printf("period size = %d frames\n", (int)frames);
  snd_pcm_hw_params_get_buffer_time(params,&val, &dir);
  printf("buffer time = %d us\n", val);
  snd_pcm_hw_params_get_buffer_size(params,(snd_pcm_uframes_t *) &val);
  printf("buffer size = %d frames\n", val);
  snd_pcm_hw_params_get_periods(params, &val, &dir);
  printf("periods per buffer = %d frames\n", val);
  snd_pcm_hw_params_get_rate_numden(params,&val, &val2);
  printf("exact rate = %d/%d bps\n", val, val2);
  val = snd_pcm_hw_params_get_sbits(params);
  printf("significant bits = %d\n", val);
  val = snd_pcm_hw_params_is_batch(params);
  printf("is batch = %d\n", val);
  val = snd_pcm_hw_params_is_block_transfer(params);
  printf("is block transfer = %d\n", val);
  val = snd_pcm_hw_params_is_double(params);
  printf("is double = %d\n", val);
  val = snd_pcm_hw_params_is_half_duplex(params);
  printf("is half duplex = %d\n", val);
  val = snd_pcm_hw_params_is_joint_duplex(params);
  printf("is joint duplex = %d\n", val);
  val = snd_pcm_hw_params_can_overrange(params);
  printf("can overrange = %d\n", val);
  val = snd_pcm_hw_params_can_mmap_sample_resolution(params);
  printf("can mmap = %d\n", val);
  val = snd_pcm_hw_params_can_pause(params);
  printf("can pause = %d\n", val);
  val = snd_pcm_hw_params_can_resume(params);
  printf("can resume = %d\n", val);
  val = snd_pcm_hw_params_can_sync_start(params);
  printf("can sync start = %d\n", val);
  return;
}


static void closeAudio() {
  if (aud.inhandle!=NULL){
    snd_pcm_drain(aud.inhandle);
    snd_pcm_close(aud.inhandle);
  }
  aud.inhandle=NULL;
  if (aud.outhandle!=NULL){
    snd_pcm_drain(aud.outhandle);
    snd_pcm_close(aud.outhandle);
  }
  aud.outhandle=NULL;
  aud.audio=-1;
  aud.recording=0;
  snd_config_update_free_global(); // free global configuation cache which can appear like a memory leak
}


static int openKnownAudio(int record) {
  int rc;
  unsigned int val;
  int dir=0;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hw_params=NULL;
  snd_pcm_uframes_t frames;
  char err[256];
  /* Open PCM device for recording (capture). */ 
  if (record) {
    if ((rc=snd_pcm_open(&aud.inhandle, "default",SND_PCM_STREAM_CAPTURE, 0))<0) {
	//if ((rc=snd_pcm_open(&aud.inhandle, "default",SND_PCM_STREAM_CAPTURE, 0))<0) {
      printf("unable to open pcm device for recording: %s\n",snd_strerror(rc));
    }
    handle=aud.inhandle;
  } else {
    if ((rc=snd_pcm_open(&aud.outhandle, "default",SND_PCM_STREAM_PLAYBACK, 0))<0) {
	printf("unable to open pcm device for playback: %s\n",snd_strerror(rc));
    }
    handle=aud.outhandle;
  }

  /* Configure hardware parameters  */
  if((rc=snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    sprintf(err,  "unable to malloc hw_params: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  if((rc=snd_pcm_hw_params_any(handle, hw_params))<0) {
    sprintf(err,  "unable to setup hw_params: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  if((rc=snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED))<0) {
    sprintf(err,  "unable to set access mode: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  if((rc=snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE))<0) {
    sprintf(err,  "unable to set format: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  if((rc=snd_pcm_hw_params_set_channels(handle, hw_params, 1))<0) {
    sprintf(err,  "unable to set channels: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  val = SAMPLERATE;
  dir = 0;
  if((rc=snd_pcm_hw_params_set_rate(handle, hw_params, SAMPLERATE,0))<0) {
    sprintf(err,  "unable to set samplerate: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  if (val!=SAMPLERATE) {
    sprintf(err,  "unable to set requested samplerate: requested=%i got=%i\n",SAMPLERATE,val);
    audioPanic(err);
  }
  frames = 32; 
  if ((rc=snd_pcm_hw_params_set_period_size_near(handle,hw_params, &frames, &dir))<0) {
    sprintf(err,  "unable to set period size: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
  frames = 4096; 
  if ((rc=snd_pcm_hw_params_set_buffer_size_near(handle,hw_params, &frames))<0) {
    sprintf(err,  "unable to set buffer size: %s\n",snd_strerror(rc));
    audioPanic(err);
  }

  if ((rc = snd_pcm_hw_params(handle, hw_params))<0) {
    sprintf(err,  "unable to set hw parameters: %s\n",snd_strerror(rc));
    audioPanic(err);
  }
#if 0
  aboutAlsa(handle,hw_params); // useful debugging
#endif
  snd_pcm_hw_params_free(hw_params);
  aud.recording = (record)? 1:0;
  aud.audio=1; // ready
  return 1;
}
	      

/* Copy available PCM audio data into local memory. For instance, when called frequently, this avoids overflowing the internal buffer if recognition runs slower than realtime. */
void audioEventLoop()  
{ 
  if (!aud.bUseFileInput)
  {
    char err[256];
    int wantSamples = AUDIO_BUFFERSZ / 1000. * SAMPLERATE;
    int availSamples=0;
    int rc;
    if(!aud.recording) return;
    /* wait till the interface is ready for data, or 30 second has elapsed */
    if ((rc = snd_pcm_wait(aud.inhandle, 30000)) < 0) {
      fprintf(stderr, "poll failed (%s)\n", strerror (rc));
      //return;
    }	           	
    if ((availSamples = snd_pcm_avail_update(aud.inhandle)) < 0) {
      if (availSamples == -EPIPE) {
        fprintf (stderr, "an xrun occured\n");
      } else {
        fprintf (stderr, "unknown ALSA avail update return value (%d)\n", availSamples);
      }
    } else if(availSamples>=wantSamples) {
      int gotSamples=0;
      audiol_t *p=aud.last;
      //printf("want=%i avail=%i\n",wantSamples,availSamples);
      while (availSamples-gotSamples>=wantSamples) { // copy all avail buffers
        if(p) {
          while(p->n) p=p->n;
	  p->n=memset(malloc(sizeof(audiol_t)),0,sizeof(audiol_t));
          p=p->n;
        } else  
	  p=aud.last=memset(malloc(sizeof(audiol_t)),0,sizeof(audiol_t));
        p->flags=0;
        p->len=wantSamples;
        p->audio=malloc(wantSamples*sizeof(short));
        if ((rc = snd_pcm_readi(aud.inhandle,p->audio,p->len)) < 0) {
	  sprintf(err, "read failed (%s)\n", snd_strerror (rc));
	  audioPanic(err);
        }
        gotSamples+=p->len;
        //printf("gotSamples=%i\n",gotSamples);
      }	
    } else {
      // insufficient audio... call audioEventLoop() again later
      usleep(10000); // Force thread to sleep for 10ms; allows time to service audio
    }
  }
} 


int startRecord()  
{
  char err[256];
  if (!aud.bUseFileInput) {
    int rc;
    while(aud.last) {/* first wipe old recording */
      audiol_t *p = aud.last;
      aud.last=aud.last->n;
      if(p->audio) free(p->audio);
      free(p);
    }
    aud.last=NULL;
    if(!openKnownAudio(1)) {
      printf("STARTRECORD FAILED\n");
      return 0;
    }
    if ((rc = snd_pcm_prepare(aud.inhandle)) < 0) {
      sprintf(err, "cannot prepare audio interface for use (%s)\n",snd_strerror(rc));
      audioPanic(err);
    }
    if ((rc = snd_pcm_start(aud.inhandle)) < 0) {
      sprintf(err, "cannot start PCM audio (%s)\n",snd_strerror(rc));
      audioPanic(err);
    }
    return 1;
  }
  else {
    /* aud.bUseFileInput == true */
    if (aud.sampleIndex == 0)
    {
      while (aud.fileIndex < aud.numFiles) {
        char str[512];
        sprintf(str, "FILE=%s\n", aud.fssdk_input_file[aud.fileIndex]);
        disp(aud.cons, str);

        if (!(thfWaveFromFile(aud.ses, aud.fssdk_input_file[aud.fileIndex++], &aud.waveFileBuffer,
          &aud.inlen, &aud.sampleRate)))
        {
          sprintf(str, "thfWaveFromFile failed for %s", aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          killAudio();
          exit(1);
        }
        if (aud.sampleRate != SAMPLERATE) {
          sprintf(str, "Warning: Input sampleRate(%d) differs from defined sampleRate(%d). Skipping file (%s).", 
           (int)SAMPLERATE, (int)aud.sampleRate,aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          continue;
        }
        return 0;
      }
      if (aud.fileIndex >= aud.numFiles) {
        return RECOG_NODATA;
      }
      else
        return 0;
    }
    else
      return 0;
  }
}

/* Simple blocking audio playback */
int audioPlay(short *buffer, int len)  
{
  int rc;
  char err[256];
  if(!openKnownAudio(0)) {
    printf("failed to open audio device for playback\n");
    return 0;
  }
  if ((rc = snd_pcm_prepare(aud.outhandle)) < 0) {
    sprintf(err, "cannot prepare audio interface for use (%s)\n",snd_strerror(rc));
    audioPanic(err);
  }
  if ((rc = snd_pcm_writei(aud.outhandle,buffer,len)) < 0) {
    sprintf(err, "failed to write buffer to audio device (%s)\n",snd_strerror(rc));
    audioPanic(err);
  }
  return 1;
}


int stopRecord() 
{
  if (!aud.bUseFileInput)
  {
    if(aud.recording) {
      if(aud.audio>=0) {
        closeAudio();
      }
      aud.recording=0;
    }
    return 1;
  }
  else
  {
    if (aud.waveFileBuffer != NULL)
    {
      thfFree(aud.waveFileBuffer);
      aud.waveFileBuffer = NULL;
    }
    aud.sampleIndex = 0;
    aud.inlen = 0;
    aud.sampleRate = 0;
  }
  return 0;
}

# elif defined(USE_OSS) // oss

/*-----------------------------------------------------------------*
  "OSS" audio 
 *-----------------------------------------------------------------*/


static void closeAudio() {
  if(aud.audio>=0)close(aud.audio);
  aud.audio=-1;
  aud.recording=0;
}

static int openKnownAudio(int record)
{
  if(aud.audio<0) {
    int arg;
    if((aud.audio=open("/dev/dsp",(record)?O_RDONLY:O_WRONLY,0))==-1)
      return audioPanic("Opening /dev/dsp");

    if(ioctl(aud.audio,SNDCTL_DSP_GETCAPS,&arg)==-1) {
      return audioPanic("Reading audio capabilities");
    }
    if(!(arg&DSP_CAP_TRIGGER)) {
      return audioPanic("Inadequate capabilities");
    }
    if(record) {
      arg=~PCM_ENABLE_INPUT;
      if(ioctl(aud.audio,SNDCTL_DSP_SETTRIGGER,&arg)==-1) {
	return audioPanic("Setting up record trigger");
      }
    } else {
      arg=~PCM_ENABLE_OUTPUT;
      if(ioctl(aud.audio,SNDCTL_DSP_SETTRIGGER,&arg)==-1) {
	return audioPanic("Setting up playback trigger");
      }
    }
    arg=AFMT_S16_LE;
    if(ioctl(aud.audio,SNDCTL_DSP_SETFMT,&arg)==-1) {
      return audioPanic("Setting sample format");
    }
    if(arg!=AFMT_S16_LE) {
      return audioPanic("required sample format not available");
    }
    arg=1;
    if(ioctl(aud.audio,SNDCTL_DSP_CHANNELS,&arg)==-1) {
      return audioPanic("Setting mono");
    }
    if(arg!=1) {
      return audioPanic("Mono not available");
    }
    arg=SAMPLERATE;
    if(ioctl(aud.audio,SNDCTL_DSP_SPEED,&arg)==-1) {
      return audioPanic("Setting mono");
    }
    if(arg!=SAMPLERATE) {
      return audioPanic("Required sample rate is not available");
    }


    aud.recording = (record)? 1:0;
  }
  return 1;
}

/* Copy available PCM audio data into local memory. For instance, when called frequently, this avoids overflowing the internal buffer if recognition runs slower than realtime. */
void audioEventLoop() 
{ 
  if (!aud.bUseFileInput && aud.recording) {
    fd_set fds;
    struct timeval to = {0,0};
    FD_ZERO(&fds);
    FD_SET(aud.audio,&fds);
    if(!aud.last) to.tv_sec=1;
    if(select(aud.audio+1,&fds,NULL,NULL,&to)) {
      audiol_t *p=aud.last;
      audio_buf_info inf;
      if(ioctl(aud.audio,SNDCTL_DSP_GETISPACE,&inf)==-1) {
	(void)audioPanic("Error while recording");
	return;
      }
      while ((unsigned)inf.bytes>=AUDIO_BUFFERSZ*sizeof(short)) {
	if(p) {
	  while(p->n) p=p->n;
	  p->n=memset(malloc(sizeof(audiol_t)),0,sizeof(audiol_t));
	  p=p->n;
	} else  
	  p=aud.last=memset(malloc(sizeof(audiol_t)),0,sizeof(audiol_t));
	p->audio=malloc((p->len=AUDIO_BUFFERSZ)*sizeof(short));
	if(read(aud.audio,p->audio,p->len*sizeof(short))!=
	   (int)(p->len*sizeof(short))) {
	  (void)audioPanic("Error while recording");
	  return;
	}
	p->flags=0;
	inf.bytes -= p->len*sizeof(short);
      }
    }
  }
} 


int startRecord()
{
  char err[256];
  if (!aud.bUseFileInput) {
    int arg;
    while(aud.last) {/* first wipe old recording */
       audiol_t *p = aud.last;
      aud.last=aud.last->n;
      if(p->audio) free(p->audio);
      free(p);
    }
    aud.last=NULL;
    if(!openKnownAudio(1)) return 0;
    arg=PCM_ENABLE_INPUT;
    if(ioctl(aud.audio,SNDCTL_DSP_SETTRIGGER,&arg)==-1) {
      return audioPanic("Setting up record trigger");
    }
    return 1;
  }
  else {
    /* aud.bUseFileInput == true */
    if (aud.sampleIndex == 0)
    {
      while (aud.fileIndex < aud.numFiles) {
        char str[512];
        sprintf(str, "FILE=%s\n", aud.fssdk_input_file[aud.fileIndex]);
        disp(aud.cons, str);

        if (!(thfWaveFromFile(aud.ses, aud.fssdk_input_file[aud.fileIndex++], &aud.waveFileBuffer,
          &aud.inlen, &aud.sampleRate)))
        {
          sprintf(str, "thfWaveFromFile failed for %s", aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          killAudio();
          exit(1);
        }
        if (aud.sampleRate != SAMPLERATE) {
          sprintf(str, "Warning: Input sampleRate(%d) differs from defined sampleRate(%d). Skipping file (%s).", 
           (int)SAMPLERATE, (int)aud.sampleRate,aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          continue;
        }
        return 0;
      }
      if (aud.fileIndex >= aud.numFiles) {
        return RECOG_NODATA;
      }
    }
    else
      return 0;
  }
}

int stopRecord()
{
  if (!aud.bUseFileInput)
  {
    if(aud.recording) {
      if(aud.audio>=0) closeAudio();
      aud.recording=0;
#if SOUND_VERSION > 0x030903
      {
        audio_errinfo errinfo;
        if(ioctl(aud.audio,SNDCTL_DSP_GETERROR,&errinfo)>=0)
        if(errinfo.rec_overruns)
          return audioPanic("Recording overrun");
      }
#endif
    }
    return 1;
  }
  else
  {
    if (aud.waveFileBuffer != NULL)
    {
      thfFree(aud.waveFileBuffer);
      aud.waveFileBuffer = NULL;
    }
    aud.sampleIndex = 0;
    aud.inlen = 0;
    aud.sampleRate = 0;
  }
  return 0;
}
/* alsa/oss end */
#endif

static char** get_env_values(char* input, unsigned short *size)
{
  // The limit is VLimit number of individual input files
  size_t VLimit = 500;
  char seps[] = ";";
  char** retVals = NULL;
  const char* val;

  *size = 0;
  if (!input)
    return NULL;

  retVals = (char**)malloc(sizeof(char*)*VLimit);

  val = strtok_s(input, seps, &nextVal);
  while (val != NULL)
  {
    if ((*size) >= VLimit)
      break; // Input vals list exceeds allocated memory.

    // Trim leading space
    while (isspace(*val)) val++;
    if (*val != 0)  // All spaces?
    {
      // Trim trailing space
      char* end = (char*)(val + strlen(val) - 1);
      while (end > val && isspace(*end)) end--;
      end++;
      end[0] = 0;

      retVals[(*size)++] = (char*)val;
    }
    val = strtok_s(NULL, seps, &nextVal);
  }
  return retVals;
}


int initAudio_ex(void *blah, void *c, unsigned short *file_input)
{
  int r = initAudio(blah, c);
  *file_input = aud.bUseFileInput;
  return r;
}


int initAudio(void *blah, void *c)
{
  char* pEnvFssdkInputFileString = NULL;

  memset(&aud,0,sizeof(aud));
  aud.cons=c; aud.audio=-1;
  aud.bUseFileInput = 0;
  aud.numFiles = 0;
  aud.sampleIndex = 0;
  aud.fileIndex = 0;
  aud.waveFileBuffer = NULL;
  aud.inlen = 0;
  aud.sampleRate = 0;
  aud.ses = NULL;

  pEnvFssdkInputFileString = getenv("FSSDK_INPUT_FILES");
  if (pEnvFssdkInputFileString)
  {
    // getenv returns a pointer to the environment table entry containing FSSDK_INPUT_FILES.
    // It is not safe to modify the value of the environment variable using the returned pointer.
    // So copy it to a local static location.
    aud.fssdk_input_file_string = strcpy(malloc(strlen(pEnvFssdkInputFileString) + 1), pEnvFssdkInputFileString);
    aud.fssdk_input_file = get_env_values(aud.fssdk_input_file_string, &aud.numFiles);

    if (aud.fssdk_input_file != NULL && aud.numFiles > 0)
    {
      aud.bUseFileInput = 1;

      /* Create SDK session for use with thfGetWaveFromFile */
      if (!(aud.ses = thfSessionCreate())) {
        panic(aud.cons, "thfSessionCreate", thfGetLastError(NULL), 0);
        return 1;
      }
    }
  }

  /* Check playback */
  if(!openKnownAudio(0)) {
    fprintf(stderr,"Check playback failed\n");fflush(stderr);
    return 0;
  }
  closeAudio();
  /* Check record */
  if(!openKnownAudio(1)) {
    fprintf(stderr,"Check record failed\n");fflush(stderr);
    return 0;
  }
  closeAudio();
  return 1;
}


void killAudio()
{
  if(aud.audio>=0) closeAudio();
  while(aud.last) {/* wipe old recording */
    audiol_t *p = aud.last;
    aud.last=aud.last->n;
    if(p->audio) free(p->audio);
    free(p);
  }

  if (aud.fssdk_input_file != NULL) {
    free(aud.fssdk_input_file);
    aud.fssdk_input_file = NULL;
    aud.numFiles = 0;
  }
  if (aud.fssdk_input_file_string != NULL) {
    free(aud.fssdk_input_file_string);
    aud.fssdk_input_file_string = NULL;
  }

  if (aud.ses) {
    thfSessionDestroy(aud.ses);
    aud.ses = NULL;
  }
}

int audioPanic(char *why)
{
  panic(aud.cons,"Audio Error",why,1);
  return 0;
}

/* linux end */
#else

/* File-based audio support only */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern char **environ;

typedef struct {  
  int audio;
  int recording;
  audiol_t *last;
  void *cons;
  unsigned short bUseFileInput;
  char *fssdk_input_file_string;
  char **fssdk_input_file;
  unsigned short numFiles;
  unsigned long sampleIndex;
  unsigned short fileIndex;
  short *waveFileBuffer;
  unsigned long inlen;
  unsigned long sampleRate;
  thf_t *ses;
} audio_t;

static audio_t aud;

#undef  strtok_s
#define strtok_s(a,b,c) strtok((a),(b))

static char** get_env_values(char* input, unsigned short *size)
{
  // The limit is VLimit number of individual input files
  size_t VLimit = 500;
  char seps[] = ";";
  char** retVals = NULL;
  const char* val;

  *size = 0;
  if (!input)
    return NULL;

  retVals = (char**)malloc(sizeof(char*)*VLimit);

  val = strtok_s(input, seps, &nextVal);
  while (val != NULL)
  {
    if ((*size) >= VLimit)
      break; // Input vals list exceeds allocated memory.

    // Trim leading space
    while (isspace(*val)) val++;
    if (*val != 0)  // All spaces?
    {
      // Trim trailing space
      char* end = (char*)(val + strlen(val) - 1);
      while (end > val && isspace(*end)) end--;
      end++;
      end[0] = 0;

      retVals[(*size)++] = (char*)val;
    }
    val = strtok_s(NULL, seps, &nextVal);
  }
  return retVals;
}


int initAudio(void *blah, void *c)
{
  char* pEnvFssdkInputFileString = NULL;

  memset(&aud,0,sizeof(aud));
  aud.cons=c; aud.audio=-1;
  aud.bUseFileInput = 0;
  aud.numFiles = 0;
  aud.sampleIndex = 0;
  aud.fileIndex = 0;
  aud.waveFileBuffer = NULL;
  aud.inlen = 0;
  aud.sampleRate = 0;
  aud.ses = NULL;

  pEnvFssdkInputFileString = getenv("FSSDK_INPUT_FILES");
  if (!pEnvFssdkInputFileString) goto noliveaudio;

  aud.fssdk_input_file_string = strdup(pEnvFssdkInputFileString);
  aud.fssdk_input_file = get_env_values(aud.fssdk_input_file_string,
                                        &aud.numFiles);
  if (!aud.fssdk_input_file || !aud.numFiles) goto noliveaudio;
  aud.bUseFileInput = 1;
  /* Create SDK session for use with thfGetWaveFromFile */
  if (!(aud.ses = thfSessionCreate())) {
    panic(aud.cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 0;
  }
  return 1;

noliveaudio:
  panic(aud.cons, "initAudio", "Live audio not supported\n"
        "(set FSSDK_INPUT_FILES environment variable to a file list).", 1);
  return 0;
}

/* Copy available PCM audio data into local memory. For instance, when called frequently, this avoids overflowing the internal buffer if recognition runs slower than realtime. */
void audioEventLoop() {}

int initAudio_ex(void *blah, void *c, unsigned short *file_input)
{
  int r = initAudio(blah, c);
  *file_input = aud.bUseFileInput;
  return r;
}

int startRecord()
{
  if (aud.sampleIndex == 0) {
    while (aud.fileIndex < aud.numFiles) {
      char str[512];
      sprintf(str, "FILE=%s\n", aud.fssdk_input_file[aud.fileIndex]);
      disp(aud.cons, str);
      
      if (!(thfWaveFromFile(aud.ses, aud.fssdk_input_file[aud.fileIndex++],
                            &aud.waveFileBuffer,
                            &aud.inlen, &aud.sampleRate)))
        {
          sprintf(str, "thfWaveFromFile failed for %s",
                  aud.fssdk_input_file[aud.fileIndex - 1]);
          panic(aud.cons, str, thfGetLastError(aud.ses), 0);
          killAudio();
          exit(1);
        }
      if (aud.sampleRate != SAMPLERATE) {
        sprintf(str, "Warning: Input sampleRate(%d) differs from defined sampleRate(%d). Skipping file (%s).", 
                (int)SAMPLERATE, (int)aud.sampleRate,aud.fssdk_input_file[aud.fileIndex - 1]);
        panic(aud.cons, str, thfGetLastError(aud.ses), 0);
        continue;
      }
      return 0;
    }
    if (aud.fileIndex >= aud.numFiles) {
      return RECOG_NODATA;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

int stopRecord() 
{
  if (aud.waveFileBuffer != NULL) {
    thfFree(aud.waveFileBuffer);
    aud.waveFileBuffer = NULL;
  }
  aud.sampleIndex = 0;
  aud.inlen = 0;
  aud.sampleRate = 0;
  return 0;
}

void killAudio()
{
  while(aud.last) {/* wipe old recording */
    audiol_t *p = aud.last;
    aud.last=aud.last->n;
    if(p->audio) free(p->audio);
    free(p);
  }

  if (aud.fssdk_input_file != NULL) {
    free(aud.fssdk_input_file);
    aud.fssdk_input_file = NULL;
    aud.numFiles = 0;
  }
  if (aud.fssdk_input_file_string != NULL) {
    free(aud.fssdk_input_file_string);
    aud.fssdk_input_file_string = NULL;
  }
  if (aud.ses) {
    thfSessionDestroy(aud.ses);
    aud.ses = NULL;
  }
}


#endif


audiol_t *audioGetNextBuffer(unsigned short* done)
{
  audiol_t *p=NULL;
  if (!aud.bUseFileInput) {
    if (aud.last) {
      p = aud.last; aud.last = p->n;
    }
  }
  else 
  {
    if ((*done) == 1)
    {
      // The status update from the previous sfs...(...,&status)
      // was a status value that triggered the setting of (*done)
      // to 1 (e.g. in RecogPipe this happens when the status is 
      // neither RECOG_QUIET nor RECOG_SOUND). The critical thing 
      // is that this can happen before all the data from the file
      // has been read in.
      // StopRecord() was already called
      p = NULL;
    }
    else 
    {
      /* we assume that the buffer is smaller than a short in size */
      unsigned short plen = (unsigned short)((SAMPLERATE*AUDIO_BUFFERSZ*sizeof(short)/NUMCHANNELS/1000)/sizeof(short));
      /* aud.inlen == Number of speech samples in aud.waveFileBuffer */
      unsigned long samplesToProcess = (unsigned long)(aud.inlen - aud.sampleIndex);

      aud.last = NULL;
      samplesToProcess = (samplesToProcess > plen ? plen : samplesToProcess);
      if (!samplesToProcess && (*done) == 0)
      {
        p = NULL;
        stopRecord();
#ifdef _ONE_FILE_PER_INPUT_DEVICE_INSTANCE
        // This means that only one file per input device instance
        // will be processed; so, even files with NO trigger phrase
        // will advance phraseSpot to the next instance.
        (*done) = RECOG_NODATA;
#else
        if (startRecord() != 0)
          // There are no files left to read in and process
          (*done) = RECOG_NODATA;
#endif
      }
      else
      {
        long i, j;
        p = memset(malloc(sizeof(audiol_t)), 0, sizeof(audiol_t));
        p->len = (unsigned short)samplesToProcess;
        p->audio = malloc(p->len*sizeof(short));
        for (i = 0, j = aud.sampleIndex; i < p->len; i++, j += NUMCHANNELS)
          p->audio[i] = aud.waveFileBuffer[j];
        aud.sampleIndex = j;
        p->flags = 0;
      }
    }
  }
  return p;
}

void audioNukeBuffer(audiol_t *p)
{
  if(p) {if(p->audio) free(p->audio); free(p);}
}
