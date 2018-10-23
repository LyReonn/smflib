/*
 * SMFlib_general_filter.c
 *
 *  Created on: 2017-01-20
 *      Author: LyReonn
 */

#include <stdlib.h>
#include "SMFlib.h"
#include "SMFlib_general.h"

#define CH	evt->chn
#define D1	evt->data[1]
#define D2	evt->data[2]

METASTAT mstat;
TRKSTAT  tstat[MAX_TRK+1];
CHNSTAT  cstat[MAX_CHN+1];
WORD    _cflg;

// If locked, all event pointer, count and flag cannot be written.
BOOL statlock;

/****************************************************************/

eEVTFILTER EventFilter(const SMFEVT *evt)
{
	if (FAIL == VeriEvt(evt))
		return EF_DISCARD;

	WORD t = evt->trk;

	if (t > MAX_TRK)
		return EF_DISCARD;

	switch (evt->type)
	{
	case ET_META:
		switch (evt->mtype)
		{
		case MT_SEQNUM:
			if (FAIL == statlock || mstat.SeqNum)
				return EF_DISCARD;

			mstat.SeqNum = evt;
			return EF_TRK1BEG;

		case MT_OFFSET:
			if (FAIL == statlock || mstat.Offset)
				return EF_DISCARD;

			mstat.Offset = evt;
			return EF_TRK1BEG;

		case MT_SETTEMPO:
			if (ReadValE(evt) == mstat.stval)
				return EF_DISCARD;

			if (statlock && NULL == mstat.SetTempo) mstat.SetTempo = evt;
			if (statlock) mstat.stcnt++;
			mstat.stval = ReadValE(evt);
			return EF_TRK1;

		case MT_TIMESIG:
			if (ReadValE(evt) == mstat.tsval)
				return EF_DISCARD;

			if (statlock && NULL == mstat.TimeSig) mstat.TimeSig = evt;
			if (statlock) mstat.tscnt++;
			mstat.tsval = ReadValE(evt);
			return EF_TRK1;

		case MT_MARKER:
			if (statlock) mstat.mkcnt++;
			return EF_TRK1;

		case MT_TRKNAME:
			if (FAIL == statlock || tstat[t].TrkName)
				return EF_DISCARD;

			tstat[t].TrkName  = evt;
			return EF_TRKBEG;

		case MT_ENDOFTRK:
			if (FAIL == statlock || tstat[t].EndOfTrk)
				return EF_DISCARD;

			tstat[t].EndOfTrk = evt;
			return EF_TRKEND;

		case MT_CHNFIX:
		case MT_PORTFIX:
		case MT_INSTNAME:
		case MT_KEYSIG:
		case MT_TEXT:
		case MT_LYRIC:
		case MT_CUEPOINT:
		case MT_COPYRIGHT:
		case MT_SEQMETA:
		default:
			return EF_DISCARD;
		}
		break;

	case ET_NOTEON:
		if (TRUE == cstat[CH].nstat[D1])
			return EF_DISCARD;

		if (statlock) tstat[t].ncnt++;
		cstat[CH].nstat[D1] = TRUE;
		break;

	case ET_NOTEOFF:
		if (FAIL == cstat[CH].nstat[D1])
			return EF_DISCARD;

		cstat[CH].nstat[D1] = FAIL;
		break;

	case ET_CTRLCHG:
		if (D1 == 0x54 && CH == 9)
			return EF_DISCARD;

		if (D2 == cstat[CH].ccval[D1])
			return EF_DISCARD;

		cstat[CH].ccval[D1] = D2;
		break;

	case ET_PROGCHG:
		if (D1 == cstat[CH].pcval)
			return EF_DISCARD;

		cstat[CH].pcval = D1;
		break;

	case ET_CHNPRES:
	case ET_KEYPRES:
	case ET_PITCHBD:
		break;

	case ET_SYSEXF0:
	case ET_SYSEXF7:
		return EF_KEEP;

	default:
		return EF_DISCARD;
	}

	if (statlock)
	{
		tstat[t].mcnt++;
		tstat[t].cflg |= 1 << CH;
		_cflg |= tstat[t].cflg;
	}

	return EF_KEEP;
}

void ResetFilterLastVal(void)
{
	mstat.stval = 0;
	mstat.tsval = 0;

	for (BYTE c = 0; c <= MAX_CHN; c++)
	{
		cstat[c].pcval = INVALID;

		for (BYTE cc = 0; cc <= MAX_VAL; cc++)
			cstat[c].ccval[cc] = INVALID;

		for (BYTE n = 0; n <= MAX_VAL; n++)
			cstat[c].nstat[n] = FAIL;
	}
}

void ResetFilterStat(void)
{
	mstat.SetTempo = NULL;
	mstat.TimeSig  = NULL;
	mstat.Offset = NULL;
	mstat.SeqNum = NULL;
	mstat.stcnt  = 0;
	mstat.tscnt  = 0;
	mstat.mkcnt  = 0;

	for (WORD t = 0; t <= MAX_TRK; t++)
	{
		tstat[t].TrkName  = NULL;
		tstat[t].EndOfTrk = NULL;
		tstat[t].mcnt = 0;
		tstat[t].ncnt = 0;
		tstat[t].cflg = 0;
	}

	_cflg = 0;

	statlock = TRUE;
}

void LockFilterStat(void)
{
	statlock = FAIL;
}

BOOL GetFilterStat(void)
{
	return statlock;
}

/****************************************************************/

const SMFEVT *GetOffset(void)
{
	return mstat.Offset;
}

const SMFEVT *GetSeqNum(void)
{
	return mstat.SeqNum;
}

const SMFEVT *GetFirstSetTempo(void)
{
	return mstat.SetTempo;
}

const SMFEVT *GetFirstTimeSig(void)
{
	return mstat.TimeSig;
}

DWRD GetSetTempoCnt(void)
{
	return mstat.stcnt;
}

DWRD GetTimeSigCnt(void)
{
	return mstat.tscnt;
}

DWRD GetMarkerCnt(void)
{
	return mstat.mkcnt;
}

DWRD GetLastSetTempoVal(void)
{
	return mstat.stval;
}

DWRD GetLastTimeSigVal(void)
{
	return mstat.tsval;
}

WORD GetOverallChnFlg(void)
{
	return _cflg;
}

BYTE GetOverallChnCnt(void)
{
	BYTE cnt = 0;

	for (BYTE c = 0; c <= MAX_CHN; c++)
		if (1 << c & _cflg)
			cnt++;

	return cnt;
}

/****************************************************************/

const SMFEVT *GetTrkName(WORD trk)
{
	if (trk > MAX_TRK)
		return NULL;

	return tstat[trk].TrkName;
}

const SMFEVT *GetEndOfTrk(WORD trk)
{
	if (trk > MAX_TRK)
		return NULL;

	return tstat[trk].EndOfTrk;
}

DWRD GetMidiEvtCnt(WORD trk)
{
	if (trk > MAX_TRK)
		return FAIL;

	return tstat[trk].mcnt;
}

DWRD GetNoteCnt(WORD trk)
{
	if (trk > MAX_TRK)
		return FAIL;

	return tstat[trk].ncnt;
}

WORD GetChnFlg(WORD trk)
{
	if (trk > MAX_TRK)
		return FAIL;

	return tstat[trk].cflg;
}

BYTE GetChnCnt(WORD trk)
{
	if (trk > MAX_TRK)
		return FAIL;

	BYTE cnt = 0;

	for (BYTE c = 0; c <= MAX_CHN; c++)
		if (1 << c & tstat[trk].cflg)
			cnt++;

	return cnt;
}

BYTE GetChnNum(WORD trk)
{
	if (1 != GetChnCnt(trk))
		return INVALID;

//	if (cflg & (cflg - 1)) return INVALID;

	for (BYTE c = 0; c <= MAX_CHN; c++)
		if (1 << c & tstat[trk].cflg)
			return c;

	return INVALID;
}

BYTE GetCurOnNoteCnt(BYTE chn)
{
	if (chn > MAX_CHN)
		return INVALID;

	BYTE cnt = 0;

	for (BYTE n = 0; n <= MAX_VAL; n++)
		if (cstat[chn].nstat[n])
			cnt++;

	return cnt;
}
