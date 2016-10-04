/* Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: buildGrammar.c
 * Compiles a grammar specification into binary search file.
 * - Creates compiled search file: time.search
 *---------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <trulyhandsfree.h>

#include <console.h>
#include <datalocations.h>

#define SEARCHFILE  "time.raw"                      /* Compiled search */
#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")
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
  thf_t *ses=NULL;
  recog_t *r=NULL;
  searchs_t *s=NULL;
  pronuns_t *p=NULL;
  const char *ewhere=NULL, *ewhat=NULL;
  void *cons=NULL;
  char *gra = "hour    = one | two | three | four | five | six | \
                          seven | eight | nine | ten | eleven | twelve; \
               ampm    = am | pm; \
               halfday = noon | midnight; \
               subhour = thirty | oclock; \
               gra     = *sil%% (*nota | [twelve] $halfday | \
                          $hour [$subhour] [$ampm]) *sil%%;";
  unsigned short count = 20;
  const char *w0="oclock",*w1="am",*w2="pm",*w3="one",*w4="two",*w5="three",*w6="four",
    *w7="five",*w8="six",*w9="seven",*w10="eight",*w11="nine",*w12="ten",*w13="eleven",
    *w14="twelve",*w15="noon",*w16="midnight",*w17="thirty",*w18="*sil",*w19="*nota"; 
  const char *word[] = {w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12,w13,w14,w15,w16,
			w17,w18,w19};
  const char *p0=". oU . k l A1 k .",
    *p1=". eI1 . E1 m .",
    *p2=". p i1 . E1 m .",
    *p3=". w ^1 n .",
    *p4=". t u1 .",
    *p5=". T 9 i1 .",
    *p6=". f >1 9 .",
    *p7=". f aI1 v .",
    *p8=". s I1 k s .",
    *p9=". s E1 . v ix n .",
    *p10=". eI1 t .",
    *p11=". n aI1 n .",
    *p12=". t E1 n .",
    *p13=". I . l E1 . v ix n .",
    *p14=". t w E1 l v .",
    *p15=". n u1 n .",
    *p16=". m I1 d . n aI t .",
    *p17=". T 3r1 . t i .",
    *p18=". .pau .",
    *p19=". .nota .";
  const char *pronun[] = {p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19};
#ifdef __PALMOS__
  void *inst = (void*)(UInt32)cmd;
#endif

  /* Draw console */
  if (!(cons=initConsole(inst))) return 1;

  /* Create SDK session */
 if(!(ses=thfSessionCreate())) {
   panic(cons, "thfSessionCreate", thfGetLastError(NULL), 0);
   return 1;
  }

  /* Create recognizer */
  disp(cons,"Loading recognizer: " THF_DATADIR NETFILE);
  if(!(r=thfRecogCreateFromFile(ses,THF_DATADIR NETFILE,250,0xffff,NO_SDET)))
    THROW("thfRecogCreateFromFile");

  /* Create pronunciation object */
  disp(cons,"Loading pronunciation model: " THF_DATADIR LTSFILE);
  if(!(p=thfPronunCreateFromFile(ses,THF_DATADIR LTSFILE,0)))
    THROW("pronunCreateFromFile");

  /* Create search */
  disp(cons,"Compiling search...");
  if(!(s=thfSearchCreateFromGrammar(ses,r,p,gra,word,pronun,count,NBEST,NO_PHRASESPOTTING))) 
    THROW("thfSearchCreateFromGrammar");

  /* Save search */
  disp(cons,"Saving search: " SAMPLES_DATADIR SEARCHFILE);  
  if(!(thfSearchSaveToFile(ses,s,SAMPLES_DATADIR SEARCHFILE))) 
    THROW("thfSearchSaveToFile");
  
  /* Clean Up */
  thfPronunDestroy(p);   
  thfSearchDestroy(s);
  thfRecogDestroy(r);
  thfSessionDestroy(ses);

  /* Wait on user */
  disp(cons,"Done");
  closeConsole(cons);
  return 0;

  /* Async error handling, see console.h for panic */
 error:
  if(!ewhat && ses) ewhat=thfGetLastError(ses);
  if(s) thfSearchDestroy(s);
  if(r) thfRecogDestroy(r);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1; 
}
