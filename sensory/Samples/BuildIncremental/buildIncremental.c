/* Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: buildList.c
 * Compiles a vocabulary text file into binary search file.
 *   - Creates compiled search file: names.search
 *---------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <datalocations.h>

/* set this flag to create pronunciations programatically */
#define SEARCHFILE "incremental.raw"                     /* Compiled search */

#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")
#define NBEST 3

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
  recog_t *r=NULL;
  searchs_t *s=NULL;
  pronuns_t *p=NULL;
  thf_t *ses=NULL;
  const char *ewhere, *ewhat=NULL;
  void *cons=NULL;
  const char *addGrams1[] = {"g=*sil call John Smith [at (cell | home | work)] *sil;", 
			     "g=*sil call Jane Doe [at home] *sil;"};
  const char *words1[] = {"call", "John", "Smith", "Jane", "Doe", "at", "cell", 
			  "home", "work", "*sil"};
  /* remGrams2 specifies what gets removed on the second call to thfSearchCreateIncremental.  
     NOTE: if we were to remove "g=*sil call John Smith [at work] *sil;", then this would remove
     both "call John Smith at work" and "call John Smith", leaving just
     "call John Smith at (cell | home);", which isn't what we want. */
  const char *remGrams2[] = {"g=*sil call John Smith at work *sil;"};
  const char *addGrams2[] = {"g=*sil call Jane Doe [at work] *sil;"};
  const char *words2[] = {"call", "John", "Smith", "Jane", "Doe", "at", "work", "*sil"};
#ifdef __PALMOS__
  void *inst = (void*)(UInt32)cmd;
#endif
  /* Initialise console */
  if (!(cons=initConsole(inst))) return 1;

  /* Create session */
  if(!(ses=thfSessionCreate())) {
    panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
    return 1;
  }

  /* Create recognizer */
  disp(cons,"Loading recognizer: " THF_DATADIR NETFILE);
  if(!(r=thfRecogCreateFromFile(ses,THF_DATADIR NETFILE,(unsigned short)250,
				(unsigned short)-1,NO_SDET))) 
    THROW("recogCreateFromFile");

  /* Create pronunciation object */
  disp(cons,"Loading pronunciation model: " THF_DATADIR LTSFILE);
  if(!(p=thfPronunCreateFromFile(ses,THF_DATADIR LTSFILE,0)))
    THROW("pronunCreateFromFile");

  /* Adding addGrams1 */
  s = thfSearchCreateIncremental(ses,r,NULL,p,NULL,0,addGrams1, 2, words1, 
				 NULL, 10, NBEST);
  if (!s)
    THROW("searchCreateIncremental");
  /* Removing remGrams2 and then adding addGrams2 */
  s = thfSearchCreateIncremental(ses,r,s,p,remGrams2,1,addGrams2,1,words2,NULL,
				 8,NBEST);
  if (!s)
    THROW("searchCreateIncremental");
  /* Save search */  
  disp(cons,"Saving search: " SAMPLES_DATADIR SEARCHFILE);
  if(!(thfSearchSaveToFile(ses,s,SAMPLES_DATADIR SEARCHFILE))) 
    THROW("searchSaveToFile");
  
  /* Clean Up */
  thfPronunDestroy(p);    p=NULL;
  thfRecogDestroy(r);     r=NULL;
  thfSearchDestroy(s);    s=NULL;
  thfSessionDestroy(ses); ses=NULL;

  /* Wait on user */
  disp(cons,"Done");
  closeConsole(cons);
  return 0;
  
  /* Async error handling, see console.h for panic */
 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(p)   thfPronunDestroy(p);
  if(r)   thfRecogDestroy(r);
  if(s)   thfSearchDestroy(s);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1; 
}
