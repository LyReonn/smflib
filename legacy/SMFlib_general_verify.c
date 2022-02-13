/*
 * SMFlib_general_verify.c
 *
 *  Created on: 2017-01-20
 *      Author: LyReonn
 */

#include <string.h>
#include "SMFlib.h"
#include "SMFlib_general.h"

BOOL VeriBuf(const SMFILE *smf)
{
	const BYTE *ptr[1] = {NULL};
	WORD fmt  = 0, tcnt = 0;
	DWRD hlen = 0, tlen = 0;
	SIZE size = 0;

//	VERI_INVALID_POINTER
	if (NULL == smf
//	VERI_INVALID_FIELD
	||	NULL == smf->buf
//	VERI_INVALID_DATA
	||	smf->size < 14)
		return FAIL;

	ptr[0] = smf->buf;

	if (strcmp(ReadStr(ptr, LEN), "MThd"))
//		VERIBUF_INVALID_BUFFER
		return FAIL;

	hlen = ReadVal(ptr, LEN);
	fmt  = ReadVal(ptr, 2);
	tcnt = ReadVal(ptr, 2);

	if (0 == ReadVal(ptr, 2)
	||	hlen < 6
	||	fmt  > 1
	||	0 == tcnt
	|| (0 == fmt && tcnt > 1))
//		VERIBUF_INVALID_BUFFER
		return FAIL;

	size += LEN * 2 + hlen;
	SkipBuf(ptr, hlen - 6);

	for (WORD t = 1; t <= tcnt; t++)
	{
		if (strcmp(ReadStr(ptr, LEN), "MTrk"))
//			VERIBUF_INVALID_BUFFER
			return FAIL;

		tlen = ReadVal(ptr, LEN);
		size += LEN * 2 + tlen;
		SkipBuf(ptr, tlen - 3);

		if (ReadVal(ptr, 3) != DV_ENDOFTRK)
//			VERIBUF_INVALID_BUFFER
			return FAIL;
	}

	if (smf->size != size)
//		VERIBUF_SIZE_MISMATCH
		return FAIL;

	return TRUE;
}

BOOL VeriBufSmf(const SMFILE *smf)
{
	extern BOOL GetBasicInfo(SMFILE *);
	SMFILE tmp;

	if (NULL == smf)
//		VERI_INVALID_POINTER
		return FAIL;

	tmp.buf  = smf->buf;
	tmp.size = smf->size;

	if (FAIL == GetBasicInfo(&tmp)
	||	smf->fmt  != tmp.fmt
	||	smf->div  != tmp.div
	||	smf->tcnt != tmp.tcnt
	||	smf->ecnt != tmp.ecnt)
//		VERIBUFSMF_MISMATCH
		return FAIL;

	return TRUE;
}

BOOL VeriSmf(const SMFILE *smf)
{
	const SMFEVT *ptr = NULL;
	WORD trkm = 0;

//	VERI_INVALID_POINTER
	if (NULL == smf
//	VERI_INVALID_FIELD
	||	NULL == smf->evt
	||	0 == smf->div
	||	0 == smf->ecnt
	||	0 == smf->tcnt)
		return FAIL;

	if (smf->fmt > 1
	|| (0 == smf->fmt && smf->tcnt > 1))
//		VERI_INVALID_DATA
		return FAIL;

	ptr = smf->evt;

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
	{
		if (FAIL == VeriEvt(ptr))
//			VERISMF_INVALID_EVENT
			return FAIL;

		if (ptr->trk > trkm)
			trkm = ptr->trk;
	}

	if (trkm > smf->tcnt)
//		VERISMF_TRACK_EXCEED
		return FAIL;

	return TRUE;
}

BOOL VeriEvtOrd(const SMFILE *smf)
{
	const SMFEVT *ptr = NULL;
	DWRD tick = 0;
	WORD trkm = 0;

//	VERI_INVALID_POINTER
	if (NULL == smf
//	VERI_INVALID_FIELD
	||	NULL == smf->evt
	||	0 == smf->div
	||	0 == smf->ecnt
	||	0 == smf->tcnt)
		return FAIL;

	if (smf->fmt > 1
	|| (0 == smf->fmt && smf->tcnt > 1))
//		VERI_INVALID_DATA
		return FAIL;

	ptr = smf->evt;

	for (DWRD i = 0; i < smf->ecnt; i++, ptr++)
	{
//		if (FAIL == VeriEvt(ptr)) return FAIL;

		if (ptr->trk < trkm)
//			VERIORD_TRACK_OUT_OF_ORDER
			return FAIL;

		if (ptr->trk > trkm)
		{
			trkm++;
			tick = 0;

			if (ptr->trk != trkm)
//				VERIORD_TRACK_DISCONTINUITY
				return FAIL;
		}

		if (ptr->tick < tick)
//			VERIORD_TICK_OUT_OF_ORDER
			return FAIL;

		tick = ptr->tick;
	}

	if (smf->tcnt != trkm)
//		VERIORD_TRACK_COUNT_MISMATCH
		return FAIL;

	return TRUE;
}

BOOL VeriEvt(const SMFEVT *evt)
{
	if (NULL == evt)
//		VERI_INVALID_POINTER
		return FAIL;

//	VERIEVT_INVALID_DATA
	switch (evt->type)
	{
	case ET_SYSEXF0:
	case ET_SYSEXF7: break;

	case ET_META:
		switch (evt->mtype)
		{
		case MT_OFFSET:		if (0 != evt->tick || 1 != evt->trk || 5 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_SEQNUM:		if (0 != evt->tick || 1 != evt->trk || 2 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_COPYRIGHT:
		case MT_TRKNAME:
		case MT_INSTNAME:	if (0 != evt->tick) return FAIL; break;

		case MT_TIMESIG:	if (4 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_SETTEMPO:	if (3 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_KEYSIG:		if (2 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_CHNFIX:		if (1 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_PORTFIX:	if (1 != evt->len || NULL == evt->data) return FAIL; break;
		case MT_ENDOFTRK:	if (0 != evt->len || NULL != evt->data) return FAIL; break;

		case MT_TEXT:
		case MT_LYRIC:
		case MT_MARKER:
		case MT_CUEPOINT:
		case MT_SEQMETA: break;

//		VERIEVT_INVALID_DATA
		default: return FAIL;
		}
		break;

	case ET_NOTEOFF:
	case ET_NOTEON:
	case ET_KEYPRES:
	case ET_CTRLCHG:
	case ET_PITCHBD:
	case ET_PROGCHG:
	case ET_CHNPRES:
		if (0 == evt->len
		||	NULL == evt->data)
//			VERIEVT_INVALID_FIELD
			return FAIL;

//		VERIEVT_DATA_MISMATCH
		if (evt->chn != (evt->data[0] & 0xF)
		||	evt->len != GetMsgLen(evt->data[0])
//		VERIEVT_INVALID_DATA
		||	evt->data[1] > MAX_VAL)
			return FAIL;

		EVTTYPE type = evt->data[0] >> 4;

		switch (evt->type)
		{
//		VERIEVT_DATA_MISMATCH
		case ET_NOTEOFF: if (ET_NOTEON == type && 0 != evt->data[2]) return FAIL;
			type = ET_NOTEOFF;
			goto VERI;
//		VERIEVT_DATA_MISMATCH
		case ET_NOTEON:  if (0 == evt->data[2]) return FAIL;
		/* no break */
		case ET_KEYPRES:
		case ET_CTRLCHG:
//		VERIEVT_INVALID_DATA
		case ET_PITCHBD:
VERI:		if (evt->data[2] > MAX_VAL) return FAIL;
		/* no break */
		case ET_PROGCHG:
//		VERIEVT_DATA_MISMATCH
		case ET_CHNPRES: if (evt->type != type) return FAIL; break;

		default: return FAIL;
		}
		break;

//	VERIEVT_INVALID_DATA
	default: return FAIL;
	}

	return TRUE;
}
