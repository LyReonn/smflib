/*
 * baroffset.c
 *
 *  Created on: 2017-05-12
 *      Author: LyReonn
 */

#include <math.h>
#include <stdio.h>
#include "../SMFlib.h"
#include "../SMFlib_general.h"
#include "baroffset.h"

extern BOOL FilterEvents(SMFILE *smf);

int BarOffset(SMFILE *smf)
{
	if (FAIL == FilterEvents(smf)
	||	FAIL == GetOverallChnCnt())
		return BAROFFSET_FILTER_FAILED;

	if (GetOverallChnCnt() > 2)
		return BAROFFSET_TOO_MANY_CHN;

	SMFEVT *ptr = smf->evt;

#if 1
//	Check if the first note starts on the very beginning.
//	Otherwise, the song is upbeat, no need to process.
	DWRD tickm = 0xFFFFFFF;

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
		if (ET_NOTEON == ptr->type
		&&	ptr->tick < tickm)
			tickm = ptr->tick;

	if (tickm > 0)
		return BAROFFSET_UPBEAT;
#endif

	BYTE *timesig = TIME_SIG;
	DWRD bartick = 0;

	if (GetTimeSigCnt())
		timesig = GetFirstTimeSig()->data;

	bartick = (float)smf->div * 4 / pow(2, timesig[1]) * timesig[0];
	ptr = smf->evt;

	for (BYTE t = 1; t <= smf->tcnt; t++)
		while (ptr->trk == t)
		{
			if (ET_NOTEON != ptr->type)
			{
				ptr++;
				continue;
			}
			else
				for (; ptr->trk == t; ptr++)
					ptr->tick += bartick;
		}

	return BAROFFSET_SUCCESS;
}
