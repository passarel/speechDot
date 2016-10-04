/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 * ----------------------------------------------------------------
 * Sensory TrulyHandsfree SDK: console.h
 * Provides simple console for text output.
 *-----------------------------------------------------------------
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#if defined(__cplusplus) 
extern "C" {
#endif

#define MAX_DISPLAY_LINE 1024

/* initConsole: displays the console */
#if defined(_WIN32_WCE)
# include <windows.h>
void *initConsole(HINSTANCE inst);
#elif defined(__SYMBIAN32__)
#pragma warning( disable : 4057 4100 4127 4204 4210 4221 4244 4706 )
void *initConsole(void *inst);
#else
void *initConsole(void *inst);
#endif

#include <stdarg.h>

/* closeConsole: closes the console */
void closeConsole(void *c);

/* disp: write text to the console */
void dispv(void *c, const char *format, ...);
void disp(void *c, const char *what);
void dispn(void *c, const char *what);

/* panic: displays error messagebox */
void panic(void *c, const char *where, const char *why, int exit);

#if defined(__cplusplus) 
}
#endif
#endif
