/* Sensory Confidential
 * Copyright (C) 2000-2015 Sensory, Inc.
 */

#ifndef _PHONOLOGY_TYPEDEF
#define _PHONOLOGY_TYPEDEF
#include <sensorytypes.h>
typedef struct {unsigned short a,*b;} phonElement_t;
typedef struct {unsigned short a,b,c,d,*e,*f;} phonRule_t;
typedef struct {
 unsigned short a,b,c,d,e,f,g;
 phonRule_t *h;
 phonElement_t *i;
 u32 *j, *k;
 unsigned char *l;
 u32 m;
 char *n;
} phonRulesC_t;
#endif
#ifndef _M
# define __M(a,b) a##b
# define _M(a,b) __M(a,b)
#endif
static const unsigned short _M(ph_fte,INDEX)[] = {
 0,0, 1,
 1,0, 32768,1,
 0,1, 1,32769,
 0,2, 1,
 2,0, 1,
 0,3,4, 32768,6,
 0,5,3,4, 32768,32769,6,
 0,5,5,3,4, 32768,32769,32770,6,
 0,5,5,5,3,4, 32768,32769,32770,32771,6,
 2,3,4,2, 32768,6,32771,
 2,3,4,5,2, 32768,6,32771,32772,
 2,3,4,5,5,2, 32768,6,32771,32772,32773,
 2,5,3,4,2, 32768,32769,6,32772,
 2,5,3,4,5,2, 32768,32769,6,32772,32773,
 2,5,3,4,5,5,2, 32768,32769,6,32772,32773,32774,
 2,5,5,3,4,2, 32768,32769,32770,6,32773,
 2,5,5,3,4,5,2, 32768,32769,32770,6,32773,32774,
 2,5,5,3,4,5,5,2, 32768,32769,32770,6,32773,32774,32775,
 2,5,5,5,3,4,2, 32768,32769,32771,6,32774,
 2,5,5,5,3,4,5,2, 32768,32769,32771,6,32774,32775,
 2,5,5,5,3,4,5,5,2, 32768,32769,32771,6,32774,32775,32776,
 0,3,6, 32768,17,
 0,5,3,6, 32768,32769,17,
 0,5,5,3,6, 32768,32769,32770,17,
 0,5,5,5,3,6, 32768,32769,32770,32771,17,
 2,3,6,2, 32768,17,32771,
 2,3,6,5,2, 32768,17,32771,32772,
 2,3,6,5,5,2, 32768,17,32771,32772,32773,
 2,5,3,6,2, 32768,32769,17,32772,
 2,5,3,6,5,2, 32768,32769,17,32772,32773,
 2,5,3,6,5,5,2, 32768,32769,17,32772,32773,32774,
 2,5,5,3,6,2, 32768,32769,32770,17,32773,
 2,5,5,3,6,5,2, 32768,32769,32770,17,32773,32774,
 2,5,5,3,6,5,5,2, 32768,32769,32770,17,32773,32774,32775,
 2,5,5,5,3,6,2, 32768,32769,32771,17,32774,
 2,5,5,5,3,6,5,2, 32768,32769,32771,17,32774,32775,
 2,5,5,5,3,6,5,5,2, 32768,32769,32771,17,32774,32775,32776,
 7,0,8,9,10, 32768,32769,40,32771,32772,
 7,4,0,8,9,10, 32768,32769,32770,40,32772,32773,
 11,8,2,11, 32768,40,32770,32771,
 7,4,2,8,12, 32768,32769,32770,40,32772,
 11,0,8,13,2, 32768,32769,40,32771,32772,
 14,8, 32768,41,
 15,0,16,17, 32768,32769,10,32771,
 18,0,16,17, 32768,32769,10,32771,
 19, 28,
 20, 29,
 21, 30,
 22, 31,
 23, 33,
 24, 34,
 25, 35,
 26, 36,
 27, 37,
 28, 38,
 8,1, 42,32769,
 8,2,1, 42,32769,32770,
 8,14,29, 42,32769,32770,
 8,14,30, 42,32769,32770,
 1,2,12, 32768,32769,39,32770,
 1,12, 32768,39,32769,
 12,2,12, 32768,32769,39,32770,
 14, 
 6,31, 8,32769,
 32,33, 29,32769,
 34, 43,31,
 35, 44,31,
 36, 45,32,
 0,
 2,
 1,
 3,4,
 5,
 5,7,8,9,10,11,12,13,14,15,16,
 9,
 18,19,20,21,22,23,24,25,26,27,
 10,
 3,28,29,30,31,32,33,34,4,35,36,37,38,6,
 36,37,38,3,28,29,30,31,32,33,34,4,35,5,7,8,10,11,12,13,14,15,16,1,0,39,
 3,28,29,30,31,32,33,34,4,35,36,37,38,18,19,20,21,22,23,24,25,26,27,
 3,28,29,30,31,32,33,34,4,35,36,37,38,
 34,
 1,0,
 12,14,13,
 41,
 3,28,29,30,31,32,33,34,4,35,36,37,38,6,17,
 10,11,
 18,
 19,
 20,
 21,
 22,
 23,
 24,
 25,
 26,
 27,
 8,9,10,11,12,13,14,15,16,39,40,41,42,
 5,7,
 14,16,
 28,
 10,41,11,9,
 36,
 37,
 38,
};
static const phonRule_t _M(ph_rule,INDEX)[] = {
 {0, 5, 2, 1, (unsigned short *)&_M(ph_fte,INDEX)[0], (unsigned short *)&_M(ph_fte,INDEX)[2]},
 {0, 5, 2, 2, (unsigned short *)&_M(ph_fte,INDEX)[3], (unsigned short *)&_M(ph_fte,INDEX)[5]},
 {0, 5, 2, 2, (unsigned short *)&_M(ph_fte,INDEX)[7], (unsigned short *)&_M(ph_fte,INDEX)[9]},
 {0, 5, 2, 1, (unsigned short *)&_M(ph_fte,INDEX)[11], (unsigned short *)&_M(ph_fte,INDEX)[13]},
 {0, 5, 2, 1, (unsigned short *)&_M(ph_fte,INDEX)[14], (unsigned short *)&_M(ph_fte,INDEX)[16]},
 {11, 23, 3, 2, (unsigned short *)&_M(ph_fte,INDEX)[17], (unsigned short *)&_M(ph_fte,INDEX)[20]},
 {11, 23, 4, 3, (unsigned short *)&_M(ph_fte,INDEX)[22], (unsigned short *)&_M(ph_fte,INDEX)[26]},
 {11, 23, 5, 4, (unsigned short *)&_M(ph_fte,INDEX)[29], (unsigned short *)&_M(ph_fte,INDEX)[34]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[38], (unsigned short *)&_M(ph_fte,INDEX)[44]},
 {11, 23, 4, 3, (unsigned short *)&_M(ph_fte,INDEX)[49], (unsigned short *)&_M(ph_fte,INDEX)[53]},
 {11, 23, 5, 4, (unsigned short *)&_M(ph_fte,INDEX)[56], (unsigned short *)&_M(ph_fte,INDEX)[61]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[65], (unsigned short *)&_M(ph_fte,INDEX)[71]},
 {11, 23, 5, 4, (unsigned short *)&_M(ph_fte,INDEX)[76], (unsigned short *)&_M(ph_fte,INDEX)[81]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[85], (unsigned short *)&_M(ph_fte,INDEX)[91]},
 {11, 23, 7, 6, (unsigned short *)&_M(ph_fte,INDEX)[96], (unsigned short *)&_M(ph_fte,INDEX)[103]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[109], (unsigned short *)&_M(ph_fte,INDEX)[115]},
 {11, 23, 7, 6, (unsigned short *)&_M(ph_fte,INDEX)[120], (unsigned short *)&_M(ph_fte,INDEX)[127]},
 {11, 23, 8, 7, (unsigned short *)&_M(ph_fte,INDEX)[133], (unsigned short *)&_M(ph_fte,INDEX)[141]},
 {11, 23, 7, 5, (unsigned short *)&_M(ph_fte,INDEX)[148], (unsigned short *)&_M(ph_fte,INDEX)[155]},
 {11, 23, 8, 6, (unsigned short *)&_M(ph_fte,INDEX)[160], (unsigned short *)&_M(ph_fte,INDEX)[168]},
 {11, 23, 9, 7, (unsigned short *)&_M(ph_fte,INDEX)[174], (unsigned short *)&_M(ph_fte,INDEX)[183]},
 {11, 23, 3, 2, (unsigned short *)&_M(ph_fte,INDEX)[190], (unsigned short *)&_M(ph_fte,INDEX)[193]},
 {11, 23, 4, 3, (unsigned short *)&_M(ph_fte,INDEX)[195], (unsigned short *)&_M(ph_fte,INDEX)[199]},
 {11, 23, 5, 4, (unsigned short *)&_M(ph_fte,INDEX)[202], (unsigned short *)&_M(ph_fte,INDEX)[207]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[211], (unsigned short *)&_M(ph_fte,INDEX)[217]},
 {11, 23, 4, 3, (unsigned short *)&_M(ph_fte,INDEX)[222], (unsigned short *)&_M(ph_fte,INDEX)[226]},
 {11, 23, 5, 4, (unsigned short *)&_M(ph_fte,INDEX)[229], (unsigned short *)&_M(ph_fte,INDEX)[234]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[238], (unsigned short *)&_M(ph_fte,INDEX)[244]},
 {11, 23, 5, 4, (unsigned short *)&_M(ph_fte,INDEX)[249], (unsigned short *)&_M(ph_fte,INDEX)[254]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[258], (unsigned short *)&_M(ph_fte,INDEX)[264]},
 {11, 23, 7, 6, (unsigned short *)&_M(ph_fte,INDEX)[269], (unsigned short *)&_M(ph_fte,INDEX)[276]},
 {11, 23, 6, 5, (unsigned short *)&_M(ph_fte,INDEX)[282], (unsigned short *)&_M(ph_fte,INDEX)[288]},
 {11, 23, 7, 6, (unsigned short *)&_M(ph_fte,INDEX)[293], (unsigned short *)&_M(ph_fte,INDEX)[300]},
 {11, 23, 8, 7, (unsigned short *)&_M(ph_fte,INDEX)[306], (unsigned short *)&_M(ph_fte,INDEX)[314]},
 {11, 23, 7, 5, (unsigned short *)&_M(ph_fte,INDEX)[321], (unsigned short *)&_M(ph_fte,INDEX)[328]},
 {11, 23, 8, 6, (unsigned short *)&_M(ph_fte,INDEX)[333], (unsigned short *)&_M(ph_fte,INDEX)[341]},
 {11, 23, 9, 7, (unsigned short *)&_M(ph_fte,INDEX)[347], (unsigned short *)&_M(ph_fte,INDEX)[356]},
 {63, 39, 5, 5, (unsigned short *)&_M(ph_fte,INDEX)[363], (unsigned short *)&_M(ph_fte,INDEX)[368]},
 {63, 39, 6, 6, (unsigned short *)&_M(ph_fte,INDEX)[373], (unsigned short *)&_M(ph_fte,INDEX)[379]},
 {63, 39, 4, 4, (unsigned short *)&_M(ph_fte,INDEX)[385], (unsigned short *)&_M(ph_fte,INDEX)[389]},
 {63, 39, 5, 5, (unsigned short *)&_M(ph_fte,INDEX)[393], (unsigned short *)&_M(ph_fte,INDEX)[398]},
 {63, 39, 5, 5, (unsigned short *)&_M(ph_fte,INDEX)[403], (unsigned short *)&_M(ph_fte,INDEX)[408]},
 {136, 53, 2, 2, (unsigned short *)&_M(ph_fte,INDEX)[413], (unsigned short *)&_M(ph_fte,INDEX)[415]},
 {136, 53, 4, 4, (unsigned short *)&_M(ph_fte,INDEX)[417], (unsigned short *)&_M(ph_fte,INDEX)[421]},
 {136, 53, 4, 4, (unsigned short *)&_M(ph_fte,INDEX)[425], (unsigned short *)&_M(ph_fte,INDEX)[429]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[433], (unsigned short *)&_M(ph_fte,INDEX)[434]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[435], (unsigned short *)&_M(ph_fte,INDEX)[436]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[437], (unsigned short *)&_M(ph_fte,INDEX)[438]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[439], (unsigned short *)&_M(ph_fte,INDEX)[440]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[441], (unsigned short *)&_M(ph_fte,INDEX)[442]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[443], (unsigned short *)&_M(ph_fte,INDEX)[444]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[445], (unsigned short *)&_M(ph_fte,INDEX)[446]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[447], (unsigned short *)&_M(ph_fte,INDEX)[448]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[449], (unsigned short *)&_M(ph_fte,INDEX)[450]},
 {161, 69, 1, 1, (unsigned short *)&_M(ph_fte,INDEX)[451], (unsigned short *)&_M(ph_fte,INDEX)[452]},
 {175, 87, 2, 2, (unsigned short *)&_M(ph_fte,INDEX)[453], (unsigned short *)&_M(ph_fte,INDEX)[455]},
 {175, 87, 3, 3, (unsigned short *)&_M(ph_fte,INDEX)[457], (unsigned short *)&_M(ph_fte,INDEX)[460]},
 {175, 85, 3, 3, (unsigned short *)&_M(ph_fte,INDEX)[463], (unsigned short *)&_M(ph_fte,INDEX)[466]},
 {175, 87, 3, 3, (unsigned short *)&_M(ph_fte,INDEX)[469], (unsigned short *)&_M(ph_fte,INDEX)[472]},
 {195, 103, 3, 4, (unsigned short *)&_M(ph_fte,INDEX)[475], (unsigned short *)&_M(ph_fte,INDEX)[478]},
 {195, 103, 2, 3, (unsigned short *)&_M(ph_fte,INDEX)[482], (unsigned short *)&_M(ph_fte,INDEX)[484]},
 {195, 103, 3, 4, (unsigned short *)&_M(ph_fte,INDEX)[487], (unsigned short *)&_M(ph_fte,INDEX)[490]},
 {216, 117, 1, 0, (unsigned short *)&_M(ph_fte,INDEX)[494], (unsigned short *)&_M(ph_fte,INDEX)[495]},
 {234, 135, 2, 2, (unsigned short *)&_M(ph_fte,INDEX)[495], (unsigned short *)&_M(ph_fte,INDEX)[497]},
 {259, 151, 2, 2, (unsigned short *)&_M(ph_fte,INDEX)[499], (unsigned short *)&_M(ph_fte,INDEX)[501]},
 {270, 165, 1, 2, (unsigned short *)&_M(ph_fte,INDEX)[503], (unsigned short *)&_M(ph_fte,INDEX)[504]},
 {270, 165, 1, 2, (unsigned short *)&_M(ph_fte,INDEX)[506], (unsigned short *)&_M(ph_fte,INDEX)[507]},
 {270, 165, 1, 2, (unsigned short *)&_M(ph_fte,INDEX)[509], (unsigned short *)&_M(ph_fte,INDEX)[510]},
};
static const phonElement_t _M(ph_el,INDEX)[] = {
 {1, (unsigned short *)&_M(ph_fte,INDEX)[512]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[513]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[514]},
 {2, (unsigned short *)&_M(ph_fte,INDEX)[515]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[517]},
 {11, (unsigned short *)&_M(ph_fte,INDEX)[518]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[529]},
 {10, (unsigned short *)&_M(ph_fte,INDEX)[530]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[540]},
 {14, (unsigned short *)&_M(ph_fte,INDEX)[541]},
 {26, (unsigned short *)&_M(ph_fte,INDEX)[555]},
 {23, (unsigned short *)&_M(ph_fte,INDEX)[581]},
 {13, (unsigned short *)&_M(ph_fte,INDEX)[604]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[617]},
 {2, (unsigned short *)&_M(ph_fte,INDEX)[618]},
 {3, (unsigned short *)&_M(ph_fte,INDEX)[620]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[623]},
 {15, (unsigned short *)&_M(ph_fte,INDEX)[624]},
 {2, (unsigned short *)&_M(ph_fte,INDEX)[639]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[641]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[642]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[643]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[644]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[645]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[646]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[647]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[648]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[649]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[650]},
 {13, (unsigned short *)&_M(ph_fte,INDEX)[651]},
 {2, (unsigned short *)&_M(ph_fte,INDEX)[664]},
 {2, (unsigned short *)&_M(ph_fte,INDEX)[666]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[668]},
 {4, (unsigned short *)&_M(ph_fte,INDEX)[669]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[673]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[674]},
 {1, (unsigned short *)&_M(ph_fte,INDEX)[675]},
};
static const u32 _M(ph_phone,INDEX)[] = {

 5,7,9,30,
 32,35,37,40,
 42,44,46,48,
 50,52,54,56,
 58,60,72,75,
 78,81,84,87,
 90,93,97,101,
 105,107,109,111,
 113,115,117,119,
 121,124,127,130,
 132,157,191,287,
 289,291,};
static const u32 _M(ph_dlct,INDEX)[] = {

 293,};
static const unsigned char _M(ph_flg,INDEX)[] = {

 125,125,31,0,
 0,0,0,0,
 0,0,0,0,
 0,0,0,0,
 0,};
static const char _M(ph_d,INDEX)[] = {
 "word\0" \
 ".\0" \
 ";\0" \
 "#\0" \
 "syllabic-consonant\0" \
 "&\0" \
 "ix\0" \
 "9\0" \
 "9=\0" \
 "w\0" \
 "m\0" \
 "n\0" \
 "t\0" \
 "k\0" \
 "s\0" \
 "T\0" \
 "f\0" \
 "z\0" \
 "v\0" \
 "n=\0" \
 "flapping\0" \
 ">1\0" \
 "A1\0" \
 "E1\0" \
 "I1\0" \
 "^1\0" \
 "i1\0" \
 "u1\0" \
 "aI1\0" \
 "eI1\0" \
 "oU1\0" \
 ">\0" \
 "A\0" \
 "E\0" \
 "I\0" \
 "U\0" \
 "^\0" \
 "i\0" \
 "u\0" \
 "aI\0" \
 "eI\0" \
 "oU\0" \
 "?\0" \
 "t_(\0" \
 "voiceless-aspiration\0" \
 "t_h\0" \
 "remove-stress\0" \
 "unreleased-stop\0" \
 "t_c\0" \
 "initial-glottal-stop\0" \
 "remove-boundaries\0" \
 "nasal-place-assimilation\0" \
 "cot-merger\0" \
 "split-diphthongs\0" \
 "a\0" \
 "e\0" \
 "o\0" \
 "Default\0" \
};
static const phonRulesC_t _M(phonrules,INDEX) = {
 68,
 11,
 46,
 37,
 1,
 17,
 1,
 (phonRule_t *)_M(ph_rule,INDEX),
 (phonElement_t *)_M(ph_el,INDEX),
 (u32 *)_M(ph_phone,INDEX),
 (u32 *)_M(ph_dlct,INDEX),
 (unsigned char *)_M(ph_flg,INDEX),
 301,
 (char *)_M(ph_d,INDEX),
};
#undef _M
#undef __M
#undef __M
#undef _M
#undef _MX
