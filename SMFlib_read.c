/*
 * SMFlib_read.c
 *
 *  Created on: 2016-12-26
 *      Author: LyReonn
 */

#include <stdio.h>
#include <stdlib.h>
#include "SMFlib.h"
#include "SMFlib_general.h"

BOOL  ReadFileBuf(SMFILE *smf, const char *filename);
BOOL GetBasicInfo(SMFILE *smf);
BOOL   ReadEvents(SMFILE *smf);
BOOL    ReadEvent(SMFEVT *evt, const BYTE **buf);

SMFILE *SMFRead(const char *filename)
{
	SMFILE *smf = NULL;
//	static SMFILE smf[1];

	smf = (SMFILE *)calloc(1, sizeof(SMFILE));

	if (NULL == smf
	||	FAIL == ReadFileBuf (smf, filename)
	||	FAIL == GetBasicInfo(smf)
	||	FAIL == ReadEvents  (smf))
	{
		SMFClose(smf);
		return NULL;
	}

	return smf;
}

void SMFClose(SMFILE *smf)
{
	if (NULL == smf)
		return;

	if (NULL != smf->evt)
		for (DWRD i = 0; i < smf->ecnt; i++)
		{
			free(smf->evt[i].data);
			smf->evt[i].data = NULL;
		}

	free(smf->buf);
	free(smf->evt);
	smf->buf = NULL;
	smf->evt = NULL;

	smf->fmt  = 0;
	smf->div  = 0;
	smf->tcnt = 0;
	smf->ecnt = 0;
	smf->size = 0;

	free(smf);
}

BOOL ReadFileBuf(SMFILE *smf, const char *filename)
{
	FILE *stream = NULL;

	if (NULL == smf
	||	NULL == filename)
		return FAIL;

	stream = fopen(filename, "rb");

	if (NULL == stream
	||	fseek(stream, 0, SEEK_END))
		goto FAIL;

	smf->size = ftell(stream);
	rewind(stream);

	if (-1 == smf->size
	||	 0 == smf->size)
		goto FAIL;

	smf->buf = (BYTE *)calloc(smf->size, 1);

	if (NULL == smf->buf
	||	fread(smf->buf, 1, smf->size, stream) != smf->size
	||	fclose(stream))
	{
FAIL:	fclose(stream);
		return FAIL;
	}

	return TRUE;
}

BOOL GetBasicInfo(SMFILE *smf)
{
	const BYTE *ptr[1] = {NULL};
	const BYTE *beg = NULL;
	DWRD hlen = 0;
	DWRD tlen = 0;

	if (FAIL == VeriBuf(smf))
		return FAIL;

	ptr[0] = smf->buf;
	SkipBuf(ptr, LEN);
	hlen = ReadVal(ptr, LEN);

	smf->fmt  = ReadVal(ptr, 2);
	smf->tcnt = ReadVal(ptr, 2);
	smf->div  = ReadVal(ptr, 2);

	smf->ecnt = 0;
	SkipBuf(ptr, hlen - 6);

	for (WORD t = 1; t <= smf->tcnt; t++)
	{
		SkipBuf(ptr, LEN);
		tlen = ReadVal(ptr, LEN);
		beg  = *ptr;

		while (*ptr - beg < tlen)
		{
			ReadVarLenVal(ptr);
			ReadEvent(NULL, ptr);
			smf->ecnt += 1;
		}
	}

	if (0 == smf->ecnt)
		return FAIL;

	return TRUE;
}

BOOL ReadEvents(SMFILE *smf)
{
	const BYTE *ptr[1] = {NULL};
	const BYTE *beg = NULL;
	SMFEVT *evt = NULL;
	DWRD tlen = 0;
	DWRD tick = 0;

	if (FAIL == VeriBufSmf(smf))
		return FAIL;

	evt = (SMFEVT *)calloc(smf->ecnt, sizeof(SMFEVT));
	smf->evt = evt;

	if (NULL == smf->evt)
		return FAIL;

	ptr[0] = smf->buf;
	SkipBuf(ptr, LEN);
	SkipBuf(ptr, ReadVal(ptr, LEN));

	for (WORD t = 1; t <= smf->tcnt; t++, tick = 0)
	{
		SkipBuf(ptr, LEN);
		tlen = ReadVal(ptr, LEN);
		beg  = *ptr;

		for (; *ptr - beg < tlen; evt++)
		{
			tick += ReadVarLenVal(ptr);
			evt->tick = tick;
			evt->trk  = t;

			if (FAIL == ReadEvent(evt, ptr))
				return FAIL;
		}
	}

	return TRUE;
}

BOOL ReadEvent(SMFEVT *evt, const BYTE **buf)
{
	static BYTE _stat = 0;	// Running Status
	BYTE cur = 0;

	if (NULL ==  buf
	||	NULL == *buf)
		return FAIL;

	cur = ReadBuf(buf);

	if (cur >  0xF0
	&&	cur <  0xFF
	&&	cur != 0xF7)
		return FAIL;

	if (NULL == evt)
	{
		switch (cur)
		{
		case ET_META:
			SkipBuf(buf, 1);
		/* no break */
		case ET_SYSEXF7:
		case ET_SYSEXF0:
			SkipBuf(buf, ReadVarLenVal(buf));
			break;

		default:
			if (cur & 0x80)
			{
				_stat = cur;
				SkipBuf(buf, 1);
			}

			if (3 == GetMsgLen(_stat))
				SkipBuf(buf, 1);
		}

		return TRUE;
	}

	switch (cur)
	{
	case ET_META:
		evt->mtype = ReadBuf(buf);
	/* no break */
	case ET_SYSEXF7:
	case ET_SYSEXF0:
		evt->type = cur;
		evt->len  = ReadVarLenVal(buf);
		evt->data = ReadArr(buf, evt->len);
		break;

	default:
		if (cur & 0x80)
		{
			_stat = cur;
			cur = ReadBuf(buf);
		}

		evt->type = _stat >> 4;
		evt->chn  = _stat & 0xF;
		evt->len  = GetMsgLen(_stat);
		evt->data = (BYTE *)calloc(evt->len, 1);

		if (NULL == evt->data)
			return FAIL;

		evt->data[0] = _stat;
		evt->data[1] = cur;

		if (3 == evt->len)
			evt->data[2] = ReadBuf(buf);

		if (ET_NOTEON == evt->type
		&&	0 == evt->data[2])
			evt->type = ET_NOTEOFF;
	}

	return TRUE;
}
