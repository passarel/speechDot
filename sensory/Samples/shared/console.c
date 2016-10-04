/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *-----------------------------------------------------------------
 * Sensory TrulyHandsfree SDK: console.c
 * Provides simple console for text output.
 *-----------------------------------------------------------------
 */
#include "console.h"
#if defined(_WIN32_WCE)

/* 
 *  Fix the mess with the arm emulator that has different calling conventions
 *  than on other platforms, and as advertised in the header.
 *  Yet another class act Bill! 
 */
# ifdef _X86_
#  define SHSipInfo  _fu_SHSipInfo
#  define SHGetAppKeyAssoc _fu_SHGetAppKeyAssoc
#  define SHSetAppKeyWndAssoc _fu_SHSetAppKeyWndAssoc
#  define SHInitExtraControls _fu_SHInitExtraControls
#  define SHCloseApps _fu_SHCloseApps
#  define SHCreateMenuBar _fu_SHCreateMenuBar
#  define SHSipPreference _fu_SHSipPreference
#  define SHDoneButton _fu_SHDoneButton
# endif /* _X86_ */
# include <aygshell.h>
# ifdef _X86_
#  undef SHSipInfo
#  undef SHGetAppKeyAssoc
#  undef SHSetAppKeyWndAssoc
#  undef SHInitExtraControls
#  undef SHCloseApps
#  undef SHCreateMenuBar
#  undef SHSipPreference
#  undef SHDoneButton
WINSHELLAPI BOOL __stdcall SHSipInfo(UINT uiAction, UINT uiParam, 
				     PVOID pvParam,UINT fWinIni);
BYTE __stdcall SHGetAppKeyAssoc( LPCTSTR ptszApp );
BOOL __stdcall SHSetAppKeyWndAssoc( BYTE bVk, HWND hwnd );
BOOL __stdcall SHInitExtraControls(void);
BOOL __stdcall SHCloseApps( DWORD dwMemSought );
WINSHELLAPI BOOL __stdcall SHCreateMenuBar(SHMENUBARINFO *pmbi);
BOOL __stdcall SHSipPreference(HWND hwnd, SIPSTATE st);
BOOL __stdcall SHDoneButton(HWND hwndRequester, DWORD dwState);
# endif

/* 
 * Figure out if this is a smartphone
 */
# if defined(WIN32_PLATFORM_WFSP)
#  define SMARTPHONE
#  include <tpcshell.h>
#  include <winuserm.h>
# elif defined(WIN32_PLATFORM_PSPC)
#  define POCKETPC
# endif
#endif

//# pragma comment(linker, "aygshell.lib")
#include <stdio.h>
#include <stdlib.h>

#define APPNAME _T("Sensory") 
#define APPTITLE  _T("Sensory Example")

#ifdef _WIN32_WCE
typedef struct {
  HWND hwnd, cons;
  HINSTANCE inst;
  wchar_t *dBuf;
} cons_t;
static cons_t *gcons=NULL;
#endif

void closeConsole(void *c) 
{ 
#ifdef _WIN32_WCE
  cons_t *cons=(cons_t *)c;
  MSG msg;
  while(gcons) {
    GetMessage(&msg,cons->hwnd,0,0);
    TranslateMessage(&msg); 
    DispatchMessage(&msg); 
  }
  PostQuitMessage(0);
#else
# ifndef __linux__
  fprintf(stderr,"\n[ hit Enter to continue ]");
  getchar();
# endif
#endif
}
 

void disp(void *c, const char *what) 
{ 
#ifdef _WIN32_WCE
  cons_t *cons=(cons_t *)c;
  int len=strlen(what)+1;
  cons->dBuf=realloc(cons->dBuf,(len+2)*sizeof(wchar_t));
  MultiByteToWideChar(CP_ACP,0,what,-1,cons->dBuf,len);
  wcscat(cons->dBuf,_T("\r\n"));
  SendMessage(cons->cons,EM_REPLACESEL,(WPARAM)0,(LPARAM)cons->dBuf);
#elif defined(DSP1629)
  printf("%s\n",what);
#else
  fprintf(stderr,"%s\n",what);
#endif
} 

void dispn(void *c, const char *what) 
{ 
#ifdef _WIN32_WCE
  cons_t *cons=(cons_t *)c;
  wchar_t *buf;
  MSG msg;
  int len=strlen(what)+1;
  buf=malloc((len+2)*sizeof(wchar_t));
  MultiByteToWideChar(CP_ACP,0,what,-1,buf,len);
  SendMessage(cons->cons,EM_REPLACESEL,(WPARAM)0,(LPARAM)buf);
  while(PeekMessage(&msg,cons->hwnd,0,0, PM_NOREMOVE)) { 
    if(!GetMessage(&msg,cons->hwnd,0,0)) break;
    TranslateMessage(&msg); 
    DispatchMessage(&msg); 
  }
  free(buf);
#elif defined(DSP1629)
  printf("%s",what);
#else
  fprintf(stderr,"%s",what);
#endif
}


void dispv(void *c, const char *format, ...)
{
  static char tmp[MAX_DISPLAY_LINE+1];
  va_list ap;

  va_start(ap, format);
  vsnprintf(tmp, MAX_DISPLAY_LINE, format, ap);
  dispn(c, tmp);
  va_end(ap);
}


void panic(void *c, const char *where, const char *why, int doexit) 
{ 
#ifdef _WIN32_WCE
  cons_t *cons=(cons_t *)c;
  wchar_t buf[512];
  int l;
  wcscpy(buf,_T("Fatal Error:\n")); 
  l=wcslen(buf);
  MultiByteToWideChar(CP_ACP,0,where,-1,&buf[l],512-1); 
  wcscat(buf,_T("\nReason:\n")); 
  l=wcslen(buf);
  MultiByteToWideChar(CP_ACP,0,why,-1,&buf[l],512-l); 
  MessageBoxW(NULL, buf, _T("Panic!"),
	      MB_ICONSTOP|MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_APPLMODAL); 
  if(doexit) PostQuitMessage(1);
#elif defined(DSP1629)
  printf("Panic - %s: %s\n",where,why);
  if(doexit) exit(1);
#else
  fprintf(stderr,"Panic - %s: %s\n\n",where,why);
# ifndef __linux__
  fprintf(stderr,"[ hit Enter to exit ]");
  getchar();
# endif
  if(doexit) exit(1);
#endif
} 

#ifdef _WIN32_WCE
void _closeConsole() 
{

  if(gcons) {
    if (gcons->cons) DestroyWindow(gcons->cons);
    if (gcons->hwnd) DestroyWindow(gcons->hwnd);
    if (gcons->inst) UnregisterClass(APPNAME,gcons->inst);
    if (gcons->dBuf) free(gcons->dBuf);
    free(gcons);
    gcons=NULL;
  }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, 
				LPARAM lParam)
{
  switch (uMsg) {
  case WM_COMMAND: 
    switch (LOWORD(wParam)) {
    case 1: _closeConsole();  break; /* OK Button */
    default: goto passthru;
    }
    break;
#ifdef SMARTPHONE
  case WM_KEYUP:
    switch(wParam) {
      //case VK_TRECORD: button on da side;
    case VK_TSOFT1: _closeConsole();   break;
    default: goto passthru;
    }
    break;
#endif
  case WM_CLOSE:   _closeConsole(); break;
  passthru:
  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}
#endif

#ifdef _WIN32_WCE
void *initConsole(HINSTANCE inst)
{
  WNDCLASS wc;
  int x, y, W,H,flags;
  RECT r;

  if (!gcons) gcons = memset(malloc(sizeof(cons_t)),0,sizeof(cons_t));
  gcons->inst=inst;
  /* Only start one instance of application */
  if (FindWindow(APPNAME, NULL)){
    SetForegroundWindow(FindWindow(APPNAME, NULL));
    goto error;
  }
  
  /* Get window size */
#if defined(POCKETPC)
  {
    SIPINFO si;
    int  iDelta;
    memset(&si,0,sizeof(si));
    si.cbSize=sizeof(si);
    SHSipInfo(SPI_GETSIPINFO,0,&si,0);
    iDelta = (si.fdwFlags & SIPF_ON) ? 0 : 12;
    W = si.rcVisibleDesktop.right - si.rcVisibleDesktop.left + 2;
    H = si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top - iDelta;
    x = 0;
    y=12;
  }
#else
  x = y = W = H = CW_USEDEFAULT; 
#endif

  /* Create main window */
  memset(&wc,0,sizeof(wc));
  wc.style = CS_VREDRAW | CS_HREDRAW | CS_PARENTDC;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.hInstance = inst;
  wc.lpszClassName = APPNAME; 
  wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
  if(!RegisterClass(&wc)) goto error;
  flags = WS_VISIBLE;
#if 0
# ifdef OMAP1510DC
  flags |= WS_OVERLAPPED | WS_SYSMENU;
# endif /* OMAP1510DC */
#endif
  gcons->hwnd=CreateWindowEx(0,APPNAME,APPTITLE,flags,x,y,W,H,NULL,NULL,inst,
			     NULL);  

  ShowWindow(gcons->hwnd, SW_SHOWNORMAL);
  UpdateWindow(gcons->hwnd);
  SetFocus(gcons->hwnd);

 /* Create console */
#if defined(SMARTPHONE)
  {
    SHMENUBARINFO mbi;
    memset(&mbi, 0, sizeof(SHMENUBARINFO));
    mbi.cbSize = sizeof(SHMENUBARINFO);
    mbi.hwndParent = gcons->hwnd;
    mbi.nToolBarId = 103;
    mbi.hInstRes   = gcons->inst;
    SHCreateMenuBar(&mbi);
    GetClientRect(gcons->hwnd, &r);
  }
#elif defined(POCKETPC)
  SHSipPreference(gcons->hwnd, SIP_DOWN);
  SHDoneButton(gcons->hwnd,SHDB_SHOW);
  GetWindowRect(gcons->hwnd,&r);
  r.left+=1; r.top+=1; r.right-=3; r.bottom-=25;
#else
  r.top=r.bottom=r.left=r.right=CW_USEDEFAULT;
#endif
  gcons->cons=CreateWindowEx(0,_T("edit"),NULL,ES_READONLY|ES_AUTOVSCROLL|
			     ES_MULTILINE|ES_LEFT|WS_VISIBLE|WS_CHILD|
			     WS_VSCROLL,r.left,r.top,r.right,r.bottom,
			     gcons->hwnd,NULL,inst,NULL); 

#ifdef SMARTPHONE
  { /* Get us a teeny weeny font */
    LOGFONT lf;
    HFONT hf;
    lf.lfHeight = 12;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    wsprintf(lf.lfFaceName, TEXT("Nina"));
    hf = CreateFontIndirect(&lf);
    SendMessage(gcons->cons,WM_SETFONT,(WPARAM)hf,(LPARAM)0);
  }
#endif

  return (void *)gcons;

 error:
  return NULL;
}
#else
void *initConsole(void *c)
{
  return (void *)1;
}
#endif
