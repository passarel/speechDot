/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2015 Sensory Inc.
 *
 * -----------------------------------------------------------------
 * Sensory TrulyHandsfree SDK: datalocations.h
 * Defines where data files are located on various platforms
 *-----------------------------------------------------------------
 */

#ifndef DATALOCATIONS_H
#define DATALOCATIONS_H

#if defined(__cplusplus) 
extern "C" {
#endif

/* Evaluate and stringify macro */
#define __STR(a) #a
#define _STR(a) __STR(a)

/* Avoid unwanted substitutions in DATADIR_SAMP and DATADIR paths */
#ifdef linux
# undef linux
# define linux linux
#endif
#ifdef x86_64
# undef x86_64
# define x86_64 x86_64
#endif

#ifndef DATADIR_SAMP
# define DATADIR_SAMP /Data
#endif

#ifndef DATADIR
# define DATADIR DATADIR_SAMP/../../Data
#endif

#if 1
/*------------ 16kHz large vocabulary language data ----------*/
# define LVCSR_LANGDIR en_us_16kHz_v11
# define NETFILE_LVCSR_3_500 "nn_en_us_fbank_16k_3_500_v12.0.0.raw"
# define LTSFILE_LVCSR_3_500 "lts_en_us_12.1.4.raw"
#endif

#if 0
/*------------ 8kHz language data ----------*/
/* US ENGLISH */
# define LANGDIR    en_us_8kHz_v8
# define NETROOT    nn_en_us_mfcc_8k_15_big_250_v4.7
# define LTSROOT    lts_en_us_9.3
#endif

#if 1
/*------------ 16kHz language data ----------*/
/* US ENGLISH */
# define LANGDIR   en_us_16kHz_v10
# define TTSDIR    tts_en_us_16kHz_v1
# define NETROOT   nn_en_us_mfcc_16k_15_250_v5.1.1
# define LTSROOT   lts_en_us_9.5.2b
# define VOICEROOT tts_en_us_cid_sally_v1.0
# define SVSID_AND_UDT_ROOT svsid_and_udt_1_1
# define SVSID_ROOT         svsid_1_1
# define UDT_ROOT           udt_1_1
# define PHNSEARCH_ROOT     phonemeSearch_1_5
#endif

#if 0
/* SPANISH */
# define LANGDIR    es_16kHz_v8
# define NETROOT    nn_es_mfcc_16k_15_big_250_v1.1
# define LTSROOT    lts_es_3.14
#endif

#if 0
/* UK ENGLISH */
# define LANGDIR    en_uk_16kHz_v8
# define NETROOT    nn_en_uk_mfcc_16k_15_big_250_v2.0
# define LTSROOT    lts_en_2.8.1
#endif

#if 0
/* GERMAN */
# define LANGDIR    de_16kHz_v8
# define NETROOT    nn_de_mfcc_16k_15_big_250_v3.0
# define LTSROOT    lts_de_3.3.5
#endif

#if 0
/* FRENCH */
# define LANGDIR    fr_16kHz_v8
# define NETROOT    nn_fr_mfcc_16k_15_big_250_v2.2
# define LTSROOT    lts_fr_3.12.1
#endif

#if 0
/* ITALIAN */
# define LANGDIR    it_16kHz_v8
# define NETROOT    nn_it_mfcc_16k_15_big_250_v3.2
# define LTSROOT    lts_it_1.6
#endif

#if 0
/* MANDARIN */
# define LANGDIR    zh_16kHz_v8
# define NETROOT    nn_zh_mfcc_16k_15_big_250_v2.1
# define LTSROOT    lts_zh_3.2.4
#endif

#if 0
/* JAPANESE */
# define LANGDIR    ja_16kHz_v8
# define NETROOT    nn_ja_mfcc_16k_15_big_250_v4.2
# define LTSROOT    lts_ja_5.17
#endif

#if 0
/* KOREAN */
#define LANGDIR    ko_16kHz_v8
#define NETROOT    nn_ko_mfcc_16k_15_big_250_v2.0
#define LTSROOT    lts_ko_1.0
#endif

/* -------------COMMON ---------------------------------- */
#define NETFILE    _STR(NETROOT) ".raw" /* Acoustic model */
#define LTSFILE    _STR(LTSROOT) ".raw" /* Pronunciation model */
#define VOICEFILE  _STR(VOICEROOT) ".raw" /* TTS Voice model */
#define SVSID_AND_UDT_FILE _STR(SVSID_AND_UDT_ROOT) ".raw"
#define SVSID_FILE         _STR(SVSID_ROOT) ".raw"
#define UDT_FILE           _STR(UDT_ROOT) ".raw"
#define PHNSEARCH_FILE     _STR(PHNSEARCH_ROOT) ".raw"


#define THF_DATADIR     _STR(DATADIR) "/" _STR(LANGDIR) "/"
#define THF_TTSDIR      _STR(DATADIR) "/" _STR(TTSDIR)  "/"
#define SAMPLES_DATADIR _STR(DATADIR_SAMP) "/"

#define THF_LVCSR_DATADIR _STR(DATADIR) "/" _STR(LVCSR_LANGDIR) "/"

#define STATIC_DEF(_a_,_b_) <DATADIR/_a_/_b_.c>
#define STATIC_NETFILE            STATIC_DEF(LANGDIR,NETROOT)
#define STATIC_LTSFILE            STATIC_DEF(LANGDIR,LTSROOT)
#define STATIC_VOICEFILE          STATIC_DEF(TTSDIR,VOICEROOT)
#define STATIC_SVSID_AND_UDT_FILE STATIC_DEF(LANGDIR,SVSID_AND_UDT_ROOT)
#define STATIC_SVSID_FILE         STATIC_DEF(LANGDIR,SVSID_ROOT)
#define STATIC_UDT_FILE           STATIC_DEF(LANGDIR,UDT_ROOT)
#define STATIC_PHNSEARCH_FILE     STATIC_DEF(LANGDIR,PHNSEARCH_ROOT)

#if defined(__cplusplus) 
}
#endif
#endif
