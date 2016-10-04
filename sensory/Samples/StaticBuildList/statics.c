/*
 * Sensory Confidential
 * Copyright (C)2000-2015 Sensory Inc.
 *
 *---------------------------------------------------------------------------
 * Sensory TrulyHandsfree SDK Example: statics.c
 * Loads the static recognizer and pronun objects for the sBuildList sample
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

#define INDEX 0
#include STATIC_LTSFILE
#undef INDEX
static const void *pronuns[] = {&pronun0, &phonrules0, 
#ifdef HAVE_textnorm
				 &textnorm0,
#undef HAVE_textnorm
#else
				 NULL, 
#endif
#ifdef HAVE_ortnormA
				 &ortmormA0,
#undef HAVE_ortnormA
#else
				 NULL, 
#endif
#ifdef HAVE_ortnormB
				 &ortnormB0,
#undef HAVE_ortnormB
#else
				 NULL, 
#endif
};

sdata_t pronun_foo = {5, (const void **)pronuns};


