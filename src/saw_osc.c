/* Sawtooth Oscillator with vibrato and delay effect
 * by Xavier Halgand,
 *
 * based on "blepvco" :
 *
 * blepvco - minBLEP-based, hard-sync-capable LADSPA VCOs.
 *
 * Copyright (C) 2004-2005 Sean Bolton.
 *
 * Much of the LADSPA framework used here comes from VCO-plugins
 * 0.3.0, copyright (c) 2003-2004 Fons Adriaensen.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "minblep_tables.h"
#include "saw_osc.h"

//---------------------------------------------------------------------------

extern float   		 _p, _w, _z;
extern float  	     _f [FILLEN + STEP_DD_PULSE_LENGTH];
extern uint16_t    		 _j, _init;
extern float 		 _fsam;
extern float 		 f1, f2, pass;
extern float         phase2, phase2Step;
extern uint16_t   	 audiobuff[BUFF_LEN];
extern float 	     delayline[DELAYLINE_LEN];
extern float         *readpos;
extern float         *writepos;
extern float         fdb;
//---------------------------------------------------------------------------

void
place_step_dd(float *buffer, uint16_t index, float phase, float w, float scale)
{
	float r;
	uint16_t i;

	r = MINBLEP_PHASES * phase / w;
	i = (uint16_t)(lrintf(r - 0.5f));
	r -= (float)i;
	i &= MINBLEP_PHASE_MASK;  /* extreme modulation can cause i to be out-of-range */
	/* this would be better than the above, but more expensive:
	 *  while (i < 0) {
	 *    i += MINBLEP_PHASES;
	 *    index++;
	 *  }
	 */

	while (i < MINBLEP_PHASES * STEP_DD_PULSE_LENGTH) {
		buffer[index] += scale * (step_dd_table[i].value + r * step_dd_table[i].delta);
		i += MINBLEP_PHASES;
		index++;
	}
}
//---------------------------------------------------------------------------
/*
void
place_slope_dd(float *buffer, int index, float phase, float w, float slope_delta)
{
	float r;
	int i;

	r = MINBLEP_PHASES * phase / w;
	i = lrintf(r - 0.5f);
	r -= (float)i;
	i &= MINBLEP_PHASE_MASK;  // extreme modulation can cause i to be out-of-range

	slope_delta *= w;

	while (i < MINBLEP_PHASES * SLOPE_DD_PULSE_LENGTH) {
		buffer[index] += slope_delta * (slope_dd_table[i] + r * (slope_dd_table[i + 1] - slope_dd_table[i]));
		i += MINBLEP_PHASES;
		index++;
	}
}
*/
//---------------------------------------------------------------------------

void
sawtooth_active (void)
{
	_init = 1;
	_z = 0.0f;
	_j = 0;
	memset (_f, 0, (FILLEN + STEP_DD_PULSE_LENGTH) * sizeof (float));
}

//---------------------------------------------------------------------------
void
sawtooth_runproc (uint16_t offset, uint16_t len)
{
	uint16_t    j, n;
	uint16_t  *outp;
	float  a, p, t, w, dw, z, y, dy, freq;
	uint16_t value;

	outp    = audiobuff + offset;
	freq = f1;
	p = _p;  /* phase [0, 1) */
	w = _w;  /* phase increment */
	z = _z;  /* low pass filter state */
	j = _j;  /* index into buffer _f */

	if (_init) {
		p = 0.5f;
		w = freq / _fsam;
		if (w < 1e-5) w = 1e-5;
		if (w > 0.5) w = 0.5;
		/* if we valued alias-free startup over low startup time, we could do:
		 *   p -= w;
		 *   place_slope_dd(_f, j, 0.0f, w, -1.0f); */
		_init = 0;
	}

	//a = 0.2 + 0.8 * _port [FILT][0];
	// adjust lowpass filter
	a = 0.5f;

	do
	{
		n = 16;  //  the osc freq control is undersampled 16 times

		/* insert a vibrato  */
		phase2Step = _2PI * 16.f * f2 / SAMPLERATE;
		phase2 += phase2Step;
		phase2 = (phase2 > _2PI) ? phase2 - _2PI : phase2;
		   /* modulate freq by a sine   */
		freq = f1 * (1 + 0.02f * sinf(phase2));

		len -= n;

		t = freq / _fsam;
		if (t < 1e-5) t = 1e-5;
		if (t > 0.5) t = 0.5;
		dw = (t - w) / n;

		while (n--)
		{
			w += dw;
			p += w;

			if (p >= 1.0f)
			{  /* normal phase reset */
				p -= 1.0f;
				//*syncout = p / w + 1e-20f;
				place_step_dd(_f, j, p, w, 1.0f);
			}

			_f[j + DD_SAMPLE_DELAY] += 0.5f - p;
			z += a * (_f[j] - z);

			/* insert delay effect  */
			  dy = *readpos ; // delayed y sample
			  y = pass * z + fdb*dy;
			  y = (y > 1.0f) ? 1.0f : y ;
			  y = (y < -1.0f) ? -1.0f : y ;
			  *writepos = y;

			     /* update the delay line pointers : */
			  writepos++;
			  readpos++;
			  if ((writepos - delayline) >= DELAYLINE_LEN)
			  {
			   writepos = delayline; // wrap pointer
			  }
			  if ((readpos - delayline) >= DELAYLINE_LEN)
			  {
			   readpos = delayline;  // wrap pointer
			  }

		  /* let's hear the new sample */
			value = (uint16_t)((int16_t)((32767.0f) * y ));
			*outp++ = value; // left channel sample
			*outp++ = value; // right channel sample

			if (++j == FILLEN)
			{
				j = 0;
				memcpy (_f, _f + FILLEN, STEP_DD_PULSE_LENGTH * sizeof (float));
				memset (_f + STEP_DD_PULSE_LENGTH, 0,  FILLEN * sizeof (float));
			}
		}
	}
	while (len);

	_p = p;
	_w = w;
	_z = z;
	_j = j;
}
//---------------------------------------------------------------------------
