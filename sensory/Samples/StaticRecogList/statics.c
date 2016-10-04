/*
 * Sensory Confidential
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Sample: statics.c
 * Loads the static recognizer and search objects for the staticRecogList 
 * sample
 *---------------------------------------------------------------------------
 */
#include <trulyhandsfree.h>
#include <datalocations.h>

#ifdef _WIN32
# pragma warning( disable : 4057 4310 )
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif 

#define INDEX 0
#include STATIC_NETFILE
#undef INDEX 
static const void *recogs[] = {&recog0, };
sdata_t recog_foo = {1, (const void **)recogs};


/* You must create 'names.c' as follows:
 1. Run the 'staticBuildList' sample to create 'names.raw' (a compiled vocabulary)
 2. Use the DumpRaw.exe tool to dump 'names.c' from 'names.raw'
*/

#define INDEX 0
#include "names.c"
#undef INDEX
static const void *searches[] = {&search0, };
sdata_t search_foo = {1, (const void **)searches};


