/*
 * SMFlib_wirte.c
 *
 *  Created on: 2016-12-26
 *      Author: LyReonn
 */

#include <stdio.h>
#include <stdlib.h>
#include "SMFlib.h"
#include "SMFlib_general.h"

BOOL  FilterEvents(SMFILE *smf);
BOOL ConvertFormat(SMFILE *smf);
BOOL  CalcFileSize(SMFILE *smf);
BOOL  WriteFileBuf(SMFILE *smf);

SMFILE *SMFWrite(const SMFILE *smf, BOOL fmt)
{
	SMFILE *out = NULL;
//	static SMFILE out[1];

	out = (SMFILE *)calloc(1, sizeof(SMFILE));

	if (NULL == out)
		return NULL;

	out->fmt  = smf->fmt;
	out->div  = smf->div;
	out->tcnt = smf->tcnt;
	out->ecnt = smf->ecnt;

	out->evt  = smf->evt;
	out->buf  = NULL;
	out->size = 0;

	if (FAIL == FilterEvents(out))
		return NULL;

	if (smf->fmt != fmt
	&&	FAIL == ConvertFormat(out))
		goto FAIL;

	if (FAIL == CalcFileSize(out)
	||	FAIL == WriteFileBuf(out))
	{
FAIL:	free(out->evt);
		free(out->buf);
		free(out);

		return NULL;
	}

	return out;
}

/****************************************************************/

BOOL FilterEvents(SMFILE *smf)
{
	const SMFEVT *ptr = NULL;
	SMFEVT *evt = NULL;
	DWRD ecnt = 0;

	if (FAIL == VeriEvtOrd(smf))
		return FAIL;

	ptr = smf->evt;
	ResetFilterStat();
	ResetFilterLastVal();

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
		if (EventFilter(ptr))
			continue;

#ifdef	_PRINT_FILTERED_EVENTS
		else PrintFilteredEvent(ptr);
#endif

	for (BYTE c = 0; c <= MAX_CHN; c++)
		if (GetCurOnNoteCnt(c))
			return FAIL;

	ptr = smf->evt;
	LockFilterStat();
	ResetFilterLastVal();

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
		if (EventFilter(ptr))
			ecnt++;

	if (0 == ecnt)
		return FAIL;

	evt = (SMFEVT *)calloc(ecnt, sizeof(SMFEVT));

	if (NULL == evt)
		return FAIL;

	ptr = smf->evt;
	smf->evt = evt;
	ResetFilterLastVal();

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
		if (EventFilter(ptr))
			*evt++ = *ptr;

	smf->ecnt = ecnt;

	return TRUE;
}

BOOL SortEvents(SMFILE *smf);
void SortEventsByTick(SMFILE *smf);

BOOL ConvertFormat(SMFILE *smf)
{
	SMFEVT *ptr = NULL;
	WORD trk = 0;

	if (FAIL == VeriSmf(smf)
	||	TRUE == GetFilterStat()
	||	FAIL == GetOverallChnCnt())
		return FAIL;

	if (1 == smf->fmt)
	{
		for (DWRD i = 0; i < smf->ecnt; i++)
			smf->evt[i].trk = 1;

		SortEventsByTick(smf);

		smf->fmt  = 0;
		smf->tcnt = 1;

		return TRUE;
	}

	if (GetSetTempoCnt()
	||	GetTimeSigCnt()
	||	GetMarkerCnt())
		trk++;

	if (trk)
	{
		ptr = smf->evt;
		ResetFilterLastVal();

		for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
			if (EF_TRK1 == EventFilter(ptr))
				ptr->trk = 1;
	}

	ptr = smf->evt;

	for (BYTE c = 0; c <= MAX_CHN; c++)
		if (1 << c & GetOverallChnFlg())
		{
			trk++;

			for (DWRD i = 0; i < smf->ecnt; i++)
				if (ptr[i].type < 0xF && ptr[i].chn == c)
					ptr[i].trk = trk;
		}

	smf->fmt  = 1;
	smf->tcnt = trk;

	if (FAIL == VeriEvtOrd(smf)
	&&	FAIL == SortEvents(smf))
		return FAIL;

	return TRUE;
}

void SortEventsByTick(SMFILE *smf)
{
	SMFEVT *ptr = NULL;
	SMFEVT  tmp;

//	if (FAIL == VeriSmf(smf)) return;

	ptr = smf->evt;

	for (DWRD i = 0; i < smf->ecnt - 1; i++)
		if (ptr[i].tick > ptr[i+1].tick)
		{
			int n = i;
			do {tmp = ptr[n]; ptr[n] = ptr[n+1]; ptr[n+1] = tmp;}
			while (--n >= 0 && ptr[n].tick > ptr[n+1].tick);
		}
}

BOOL SortEvents(SMFILE *smf)
{
	SMFEVT *ptr = NULL;
	SMFEVT *evt = NULL;

	if (FAIL == VeriSmf(smf))
		return FAIL;

	SortEventsByTick(smf);

	evt = (SMFEVT *)calloc(smf->ecnt, sizeof(SMFEVT));

	if (NULL == evt)
		return FAIL;

	ptr = smf->evt;
	smf->evt = evt;

	for (WORD t = 1; t <= smf->tcnt; t++)
		for (DWRD i = 0; i < smf->ecnt; i++)
			if (ptr[i].trk == t)
				*evt++ = ptr[i];

	free(ptr);

	if (FAIL == VeriEvtOrd(smf))
		return FAIL;

	return TRUE;
}

/****************************************************************/

DWRD tlen[MAX_TRK+1] = {6};
void CalcTrkLen(SMFILE *smf, WORD trk);

BOOL CalcFileSize(SMFILE *smf)
{
	if (FAIL == VeriSmf(smf)
	||	TRUE == GetFilterStat())
		return FAIL;

	for (WORD t = 1; t <= MAX_TRK; t++)
		tlen[t] = 0;

	smf->size = 0;
	smf->size += LEN * 2;
	smf->size += tlen[0];

	if (smf->tcnt > MAX_TRK)
		smf->tcnt = MAX_TRK;

	for (WORD t = 1; t <= smf->tcnt; t++)
	{
		CalcTrkLen(smf, t);
		smf->size += LEN * 2;
		smf->size += tlen[t];
	}

	return TRUE;
}

void CalcTrkLen(SMFILE *smf, WORD trk)
{
	const SMFEVT *ptr = NULL;
	int dtick = 0;
	DWRD tick = 0;
	BYTE stat = 0;							// Running Status

	if ((ptr = GetTrkName(trk)))
	{
		tlen[trk] += 3;						// 0x00 FF 03
		tlen[trk] += GetValLen(ptr->len);
		tlen[trk] += ptr->len;
	}

#ifdef	_FILL_BLANK_TRACK_NAME
		else tlen[trk] += LEN;				// 0x00 FF 03 00
#endif

#ifdef	_RESET_ALL_CONTROLLERS
		if (1 == trk)
			tlen[trk] += 13;				// GS Reset

		if (GetMidiEvtCnt(trk))
			tlen[trk] +=
			( 4								// DV_PITCHBD
			+ 4								// RESET_CTRL
			+ 3 * 12						// 12 controllers using running status
			) * GetChnCnt(trk);
#endif

	ptr = smf->evt;

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
		if (ptr->trk == trk)
		{
			dtick = ptr->tick - tick;
			dtick = (dtick > 0)? dtick: 0;
			tlen[trk] += GetValLen(dtick);

			switch (ptr->type)
			{
			case ET_META:
				tlen[trk] += 1;
			/* no break */
			case ET_SYSEXF7:
			case ET_SYSEXF0:
				stat = ptr->type;
				tlen[trk] += 1;
				tlen[trk] += GetValLen(ptr->len);
				tlen[trk] += ptr->len;
				break;

			default:
				if (ptr->data[0] == stat)
					tlen[trk] -= 1;

				tlen[trk] += ptr->len;
				stat = ptr->data[0];
			}

			tick = ptr->tick;
		}

	if ((ptr = GetEndOfTrk(trk)))
	{
		dtick = ptr->tick - tick;
		dtick = (dtick > 0)? dtick: 0;
		tlen[trk] += GetValLen(dtick);
		tlen[trk] += 3;						// 0xFF 2F 00
	}
	else
		tlen[trk] += LEN;					// 0x00 FF 2F 00
}

/****************************************************************/

void ResetStaticVar(void);
void WriteEvent(BYTE **buf, const SMFEVT *evt);

BOOL WriteFileBuf(SMFILE *smf)
{
	const SMFEVT *eptr = NULL;
	BYTE *ptr[1] = {NULL};

	if (FAIL == VeriSmf(smf))
		return FAIL;

	smf->buf = (BYTE *)calloc(smf->size, 1);

	if (NULL == smf->buf)
		return FAIL;

	ptr[0] = smf->buf;

	WriteStr(ptr, "MThd");
	WriteVal(ptr, tlen[0],   4);
	WriteVal(ptr, smf->fmt,  2);
	WriteVal(ptr, smf->tcnt, 2);
	WriteVal(ptr, smf->div,  2);

	for (WORD t = 1; t <= smf->tcnt; t++)
	{
		if (tlen[t] < LEN)
			return FAIL;

		WriteStr(ptr, "MTrk");
		WriteVal(ptr, tlen[t], 4);
		ResetStaticVar();

		if ((eptr = GetTrkName(t))) // && smf->fmt)
			WriteEvent(ptr, eptr);

#ifdef	_FILL_BLANK_TRACK_NAME
		else WriteVal(ptr, DV_TRKNAME, 4);
#endif

#ifdef	_RESET_ALL_CONTROLLERS
		if (1 == t)
			WriteArr(ptr, GS_RESET, 13);

		if (GetMidiEvtCnt(t))
			for (BYTE c = 0; c <= MAX_CHN; c++)
				if (1 << c & GetChnFlg(t))
					WriteDefaultVal(ptr, c);
#endif

		eptr = smf->evt;

		for (DWRD i = 0; i < smf->ecnt; i++, eptr++)
			if (eptr->trk == t)
				WriteEvent(ptr, eptr);

		if ((eptr = GetEndOfTrk(t)))
			WriteEvent(ptr, eptr);
		else
			WriteVal(ptr, DV_ENDOFTRK, 4);
	}

	if (FAIL == VeriBuf(smf))
		return FAIL;

	return TRUE;
}

void WriteEvent(BYTE **buf, const SMFEVT *evt)
{
	static BYTE _stat = 0;			// Running Status
	static DWRD _tick = 0;
	int dtick = 0;

	if (NULL ==  evt
	||	NULL ==  buf
	||	NULL == *buf)
	{
		_stat = 0;
		_tick = 0;
		return;
	}

	dtick = evt->tick - _tick;
	dtick = (dtick > 0)? dtick: 0;
	_tick = evt->tick;

	WriteVarLenVal(buf, dtick);

	switch (evt->type)
	{
	case ET_META:
	case ET_SYSEXF7:
	case ET_SYSEXF0:
		WriteBuf(buf, evt->type);

		if (ET_META == evt->type)
			WriteBuf(buf, evt->mtype);

		WriteVarLenVal(buf, evt->len);
		WriteArr(buf, evt->data, evt->len);

		_stat = evt->type;
		return;

	default:
		if (evt->data[0] != _stat)
		{
			WriteBuf(buf, evt->data[0]);
			_stat = evt->data[0];
		}

		for (size_t i = 1; i < evt->len; i++)
			WriteBuf(buf, evt->data[i]);
	}
}

void ResetStaticVar(void)
{
	WriteEvent(NULL, NULL);
}

void WriteDefaultVal(BYTE **buf, BYTE chn)
{
	if (chn > MAX_CHN
	||	NULL ==  buf
	||	NULL == *buf)
		return;

	WriteVal(buf, DV_PITCHBD(chn), 4);
	WriteVal(buf, RESET_CTRL(chn), 4);

	WriteVal(buf, DV_BANKSEL,  3);
	WriteVal(buf, DV_BKSELLSB, 3);
	WriteVal(buf, DV_MODWHEEL, 3);
	WriteVal(buf, DV_CHNVOL,   3);
	WriteVal(buf, DV_CHNPAN,   3);
	WriteVal(buf, DV_EXPRESS,  3);

	WriteVal(buf, DV_SUSTPD,   3);
	WriteVal(buf, DV_PORTA,    3);
	WriteVal(buf, DV_SOSTPD,   3);
	WriteVal(buf, DV_SOFTPD,   3);
	WriteVal(buf, DV_REVERB,   3);
	WriteVal(buf, DV_CHORUS,   3);
}
