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

#define VOCABFILE  "names.txt"        /* Input: vocabulary */
#define SEARCHFILE "names.raw"        /* Output: Compiled search */
#define EXPLICIT_PRONUN 1             /* Controls internal or external pronunciation generation */
#define TEXTNORMCONDITION   0x3800    /* Note: value is language dependent */
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
  char **vocab=NULL, **pronun=NULL;
  char line[1024];
  const char *ewhere, *ewhat=NULL,*str=NULL;
  FILE *fp=NULL;
  unsigned short count=0, i;
  void *cons=NULL;
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

 /* Read vocabulary file */    
  disp(cons,"Reading vocab file: " SAMPLES_DATADIR VOCABFILE);
  if(!(fp=fopen(SAMPLES_DATADIR VOCABFILE,"r"))) 
    THROW2(SAMPLES_DATADIR VOCABFILE,"Could not open file");
  vocab=NULL;
  for(count=0; !feof(fp); ) {
    size_t sz;
    line[0]=0;
    fgets(line,1024,fp); 
    if((sz=strlen(line))>0) {
      sz--;
      while(sz>0) {
	if (line[sz]=='\r' || line[sz]=='\n') 
	  line[sz--]=0;
	else break;
      }
      str=thfTextNormalize(ses,p,line,TEXTNORMCONDITION);
      sz=strlen(str);
      if (sz>0) {
	count++;
	vocab=realloc(vocab,count*sizeof(char*));
	vocab[count-1]=strcpy(malloc(sz+1),str);
      }
      thfFree((char*)str);
    }
  }
  fclose(fp); fp=NULL;

  fprintf(stdout, "vocab[0]: %s\n", vocab[0]);

#if EXPLICIT_PRONUN
  /* Create search using explicitly-generated pronunciations */
  disp(cons,"Generating pronunciations...");
  pronun=memset(malloc(count*sizeof(char*)),0,sizeof(char*)*count);
  for(i=0; i<count; i++) {
    char *pro;
    if (!(pro=(char *)thfPronunCompute(ses,p,vocab[i],1,"",NULL))) 
      THROW("pronunCompute");
    pronun[i]=strcpy(malloc(strlen(pro)+1),pro);
   disp(cons,vocab[i]); disp(cons,pro);   
  }
  disp(cons,"Compiling search...");
  if(!(s=thfSearchCreateFromList(ses,r,p,(const char**)vocab,
				 (const char**)pronun,count,NBEST))) 
    THROW("searchCreateFromList");
  for(i=0; i<count;i++) free((void *)pronun[i]); 
  free((void *)pronun);
  pronun=NULL;
#else
  /* Use automatic pronunciations */
  disp(cons,"Compiling search...");
  if(!(s=thfSearchCreateFromList(ses,r,p,(const char**)vocab,NULL,count,NBEST))) 
    THROW("searchCreateFromList");
#endif

  /* Save search */  
  disp(cons,"Saving search: " SAMPLES_DATADIR SEARCHFILE);
  if(!(thfSearchSaveToFile(ses,s,SAMPLES_DATADIR SEARCHFILE))) 
    THROW("searchSaveToFile");
  
  /* Clean Up */
  for (i=0; i<count;i++) free((void *)vocab[i]);  
  free((void *)vocab); vocab=NULL;
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
  if(fp) fclose(fp);
  if(pronun) {
    for(i=0; i<count;i++) if(pronun[i])free((void *)pronun[i]); free((void *)pronun);
  }
  if(vocab)  {
    for (i=0; i<count;i++) if(vocab[i])free((void *)vocab[i]);  free((void *)vocab);
  }
  if(p)   thfPronunDestroy(p);
  if(r)   thfRecogDestroy(r);
  if(s)   thfSearchDestroy(s);
  panic(cons,ewhere,ewhat,0);
  if(ses) thfSessionDestroy(ses);
  return 1; 
}
