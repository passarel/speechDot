/*
 * Sensory Confidential
 *
 * Copyright (C)2013-2015 Sensory Inc.
 *
 * Returns a text string with a short description of all the flags set by
 * thfCheckRecording(), thfSpeakerCheckEnrollments()
 *  and thfUdtCheckEnrollments().
 */

#include <trulyhandsfree.h>

#include <string.h>

char *checkFlagsText(int flags)
{
  struct {
    int bitfield;
    char *text;
  } problems[] = {
    {CHECKRECORDING_BITFIELD_ENERGY_MIN, "energy_min"},
    {CHECKRECORDING_BITFIELD_ENERGY_STD_DEV, "energy_std_dev"},
    {CHECKRECORDING_BITFIELD_SIL_BEG_MSEC, "sil_beg_msec"},
    {CHECKRECORDING_BITFIELD_SIL_END_MSEC, "sil_end_msec"},
    {CHECKRECORDING_BITFIELD_SNR, "snr"},
    {CHECKRECORDING_BITFIELD_RECORDING_VARIANCE, "recording_variance"},
    {CHECKRECORDING_BITFIELD_CLIPPING_PERCENT, "clipping_percent"},
    {CHECKRECORDING_BITFIELD_CLIPPING_MSEC, "clipping_msec"},
    {CHECKRECORDING_BITFIELD_POOR_RECORDINGS, "poor_recordings"},
    {CHECKRECORDING_BITFIELD_SPOT, "spot"},
    {CHECKRECORDING_BITFIELD_VOWEL_DUR, "vowel_dur"},
    {0, NULL}
  };
  static char r[1024];
  int i, n=0;

  r[0]='\0';
  for (i=0; flags && problems[i].bitfield; i++) {
    if (!(flags & problems[i].bitfield)) continue;
    flags &= ~problems[i].bitfield;
    if (n > 0) {
      if (flags) strcat(r, ", ");
      else strcat(r, " and ");
    }
    n++;
    strcat(r,problems[i].text);
  }
  return r;
}
