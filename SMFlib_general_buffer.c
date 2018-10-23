/*
 * SMFlib_general_buffer.c
 *
 *  Created on: 2016-12-26
 *      Author: LyReonn
 */

#include <stdlib.h>
#include "SMFlib.h"

SIZE GetMsgLen(BYTE statusByte)
{
	switch (statusByte >> 4)
	{
	case ET_PROGCHG:
	case ET_CHNPRES:
		return 2;

	case ET_NOTEOFF:
	case ET_NOTEON:
	case ET_KEYPRES:
	case ET_CTRLCHG:
	case ET_PITCHBD:
		return 3;

	default:
		return FAIL;
	}
}

SIZE GetValLen(DWRD varLenVal)
{
	if (varLenVal > 0xFFFFFFF)
		return FAIL;

	for (int i = 3; i > 0; i--)
		if (varLenVal >> 7 * i)
			return (i + 1);

	return 1;
}

void SkipBuf(const BYTE **buf, SIZE cnt)
{
	if (0 == cnt
	||	NULL ==  buf
	||	NULL == *buf)
		return;

	*buf += cnt;
}

/****************************************************************/

BYTE ReadBuf(const BYTE **buf)
{
	if (NULL ==  buf
	||	NULL == *buf)
		return FAIL;

	return (*(*buf)++);
}

DWRD ReadVal(const BYTE **buf, SIZE cnt)
{
	DWRD val = 0;

	if (0 == cnt
	||	cnt > LEN
	||	NULL ==  buf
	||	NULL == *buf)
		return FAIL;

	for (SIZE i = 0; i < cnt; i++)
	{
		val <<= 8;
		val += ReadBuf(buf);
	}

	return val;
}

DWRD ReadValS(const BYTE *buf, SIZE cnt)
{
	return ReadVal(&buf, cnt);
}

DWRD ReadValE(const SMFEVT *evt)
{
	return ReadValS(evt->data, evt->len);
}

DWRD ReadVarLenVal(const BYTE **buf)
{
	DWRD val = 0;

	if (NULL ==  buf
	||	NULL == *buf)
		return FAIL;

	while (**buf & 0x80)
	{
		val += ReadBuf(buf) & 0x7F;
		val <<= 7;
	}

	val += ReadBuf(buf);

	return val;
}

DWRD ReadVarLenValS(const BYTE *buf)
{
	return ReadVarLenVal(&buf);
}

BYTE *ReadArr(const BYTE **buf, SIZE cnt)
{
	BYTE *arr = NULL;

	if (0 == cnt
	||	NULL ==  buf
	||	NULL == *buf)
		return NULL;

	arr = (BYTE *)calloc(cnt, sizeof(BYTE));

	if (NULL == arr)
		return NULL;

	for (SIZE i = 0; i < cnt; i++)
		arr[i] = ReadBuf(buf);

	return arr;
}

BYTE *ReadArrS(const BYTE *buf, SIZE cnt)
{
	return ReadArr(&buf, cnt);
}

const char *ReadStr(const BYTE **buf, SIZE cnt)
{
	static char str[256];

	if (0 == cnt
	||	cnt > 0xFF
	||	NULL ==  buf
	||	NULL == *buf)
		return NULL;

	for (SIZE i = 0; i < cnt; i++)
		str[i] = ReadBuf(buf);

	str[cnt] = '\0';

	return str;
}

const char *ReadStrS(const BYTE *buf, SIZE cnt)
{
	return ReadStr(&buf, cnt);
}

/****************************************************************/

void WriteBuf(BYTE **buf, BYTE val)
{
	if (NULL ==  buf
	||	NULL == *buf)
		return;

	*(*buf)++ = val;
}

void WriteVal(BYTE **buf, DWRD val, SIZE cnt)
{
	if (0 == cnt
	||	cnt > LEN
	||	NULL ==  buf
	||	NULL == *buf)
		return;

	for (int i = cnt - 1; i >= 0; i--)
		WriteBuf(buf, val >> 8 * i);
}

void WriteValS(BYTE *buf, DWRD val, SIZE cnt)
{
	WriteVal(&buf, val, cnt);
}

void WriteVarLenVal(BYTE **buf, DWRD val)
{
	if (val > 0xFFFFFFF
	||	NULL ==  buf
	||	NULL == *buf)
		return;

	for (int i = 3; i > 0; i--)
		if (val >> 7 * i)
			for (; i > 0; i--)
				WriteBuf(buf, val >> 7 * i | 0x80);

	WriteBuf(buf, val & 0x7F);
}

void WriteVarLenValS(BYTE *buf, DWRD val)
{
	WriteVarLenVal(&buf, val);
}

void WriteArr(BYTE **buf, const BYTE *arr, SIZE cnt)
{
	if (0 == cnt
	||	NULL ==  arr
	||	NULL ==  buf
	||	NULL == *buf)
		return;

	for (SIZE i = 0; i < cnt; i++)
		WriteBuf(buf, arr[i]);
}

void WriteArrS(BYTE *buf, const BYTE *arr, SIZE cnt)
{
	WriteArr(&buf, arr, cnt);
}

void WriteStr(BYTE **buf, const char *str)
{
	if (NULL ==  str
	||	NULL ==  buf
	||	NULL == *buf)
		return;

	for (SIZE i = 0; str[i] != '\0'; i++)
		WriteBuf(buf, str[i]);
}

void WriteStrS(BYTE *buf, const char *str)
{
	WriteStr(&buf, str);
}
