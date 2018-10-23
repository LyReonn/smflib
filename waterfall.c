/*
 * GetWaterfall.c
 *
 *  Created on: 2017-02-21
 *      Author: LyReonn
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../SMFlib.h"
#include "../SMFlib_general.h"
#include "waterfall.h"

extern BOOL FilterEvents(SMFILE *smf);
extern void SortEventsByTick(SMFILE *smf);

int WriteWaterfall(const char *filename, SMFILE *smf)
{
	if (FAIL == FilterEvents(smf)
	||	FAIL == GetOverallChnCnt())
		return WATERFALL_FILTER_FAILED;

	if (GetOverallChnCnt() > 2)
		return WATERFALL_TOO_MANY_CHN;

//	Set evt->trk to 1 (right hand) or 2 (left hand):
	SMFEVT *ptr = smf->evt;
	WORD tcnt = 0;

	for (WORD t = 1; t <= smf->tcnt; t++)
		if (GetMidiEvtCnt(t))
			tcnt++;

	switch (tcnt)
	{
	case 1:
//		if (1 == GetChnCnt())
//			return WATERFALL_ONLY_ONE_CHN;

		if (GetOverallChnFlg() > 0b11)
			return WATERFALL_CHN_INCORRECT;

		for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
			if (ptr->type < 0xF)
				ptr->trk = ptr->chn + 1;

		break;

	case 2:
		tcnt = 0;

		for (WORD t = 1; t <= smf->tcnt; t++)
			if (GetMidiEvtCnt(t))
			{
				tcnt++;
				ptr = smf->evt;

				for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
					if (ptr->trk == t)
						ptr->trk = tcnt;
			}

		break;

	default:
		return WATERFALL_TOO_MANY_TRK;
	}

//	Open output text file:
	char textname[256] = {'\0'};
	size_t dotpos = 0;
	FILE  *stream = NULL;

	dotpos = strrchr(filename, '.') - filename;
	strncpy(textname, filename, dotpos);
	strcat(textname, ".txt");

	stream = fopen(textname, "w");

	if (NULL == stream)
		return WATERFALL_WRITE_FAILED;

//	Write basic information:
	BYTE *timesig = TIME_SIG;
	DWRD settempo = DV_SETTEMPO;

	if (GetTimeSigCnt())
		timesig = GetFirstTimeSig()->data;

	if (GetSetTempoCnt())
		settempo = ReadValE(GetFirstSetTempo());

	fprintf(stream, "%d %d/%d %d %d\n", (int)(6E7 / settempo), timesig[0], (int)pow(2, timesig[1]), smf->div, settempo);

//	Write waterfall:
	const SMFEVT *tmp = NULL;
	DWRD sttime = 0, sttick = 0;
	DWRD time = 0, dur = 0;

	SortEventsByTick(smf);
	ptr = smf->evt;

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
		if (ET_NOTEON == ptr->type)
		{
			tmp = ptr;

			for (DWRD j = i; j < smf->ecnt; j++, tmp++)
				if (tmp->type == ET_NOTEOFF
				&&	tmp->chn  == ptr->chn
				&&	tmp->data[1] == ptr->data[1])
				{
					time = (float)(ptr->tick - sttick) / smf->div * settempo / 1000 + sttime;
					dur  = (float)(tmp->tick - sttick) / smf->div * settempo / 1000 + sttime - time;

					fprintf(stream, "%d:%d:%d:%d\n", ptr->data[1], time, dur, ptr->trk);
					break;
				}
				else if (ET_META == tmp->type && MT_SETTEMPO == tmp->mtype)
				{
					sttime += (float)(tmp->tick - sttick) / smf->div * settempo / 1000;
					sttick = tmp->tick;
					settempo = ReadValE(tmp);
				}
		}
		else if (ET_META == ptr->type && MT_SETTEMPO == ptr->mtype)
		{
			sttime += (float)(ptr->tick - sttick) / smf->div * settempo / 1000;
			sttick = ptr->tick;
			settempo = ReadValE(ptr);
		}

	return WATERFALL_SUCCESS;
}
