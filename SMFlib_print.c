/*
 * SMFlib_print.c
 *
 *  Created on: 2017-01-20
 *      Author: LyReonn
 */

#include <math.h>
#include <stdio.h>
#include "SMFlib.h"
#include "SMFlib_general.h"

#define ENDL	printf("\n")
#define D1		ptr->data[1]
#define D2		ptr->data[2]

void SMFPrint(const SMFILE *smf)
{
	const SMFEVT *ptr = NULL;
	DWRD ncnt = 0, mcnt = 0;
	WORD cflg = 0, aflg = 0;

//	if (FAIL == VeriEvtOrd(smf)) {printf("ERROR: INVALID DATA!\n"); return;}

	printf("File Format: %d\n", smf->fmt);
	printf("Track Count: %d\n", smf->tcnt);
	printf("Division:    %d\n", smf->div);

	ptr = smf->evt;

	for (WORD t = 1; t <= smf->tcnt; t++, aflg |= cflg, cflg = 0, ncnt = 0, mcnt = 0)
	{
		printf("\nTrack: %d", t);
		printf("\nAbsTicks_|_T/Ch_|_Event_______________|_Value_______________\n");

		for (; ptr->trk == t; ptr++)
		{
			if (FAIL == VeriEvt(ptr))
			{
				printf("%8d | Err  | Invalid Event       | ", ptr->tick);
				PrintBuf(ptr); ENDL;
				continue;
			}

			switch (ptr->type)
			{
			case ET_SYSEXF0:
			case ET_SYSEXF7:	printf("%8d | SysE | System Exclusive    | ", ptr->tick); PrintBuf(ptr); ENDL; break;
			case ET_META:		printf("%8d | Meta | ", ptr->tick);
				switch (ptr->mtype)
				{
				case MT_TEXT:		printf("Text Event          | "); PrintStr(ptr); break;
				case MT_LYRIC:		printf("Lyric               | "); PrintStr(ptr); break;
				case MT_MARKER:		printf("Marker              | "); PrintStr(ptr); break;
				case MT_CUEPOINT:	printf("Cue Point           | "); PrintStr(ptr); break;
				case MT_COPYRIGHT:	printf("Copyright Notice    | "); PrintStr(ptr); break;
				case MT_INSTNAME:	printf("Instrument Name     | "); PrintStr(ptr); break;
				case MT_TRKNAME:
					if (1 == t)		printf("Sequence Name       | ");
					else			printf("Track Name          | "); PrintStr(ptr); break;

				case MT_CHNFIX:		printf("MIDI Channel Prefix | "); PrintVal(ptr); break;
				case MT_PORTFIX:	printf("MIDI Port Prefix    | "); PrintVal(ptr); break;
				case MT_SEQNUM:		printf("Sequence Number     | "); PrintVal(ptr); break;
				case MT_OFFSET:		printf("SMTPE Offset        | "); PrintBuf(ptr); break;
				case MT_SETTEMPO:	printf("Set Tempo           | "); printf("%3.3f", 6E7 / ReadValE(ptr)); break;
				case MT_TIMESIG:	printf("Time Signature      | "); printf("%d/%d", ptr->data[0], (int)pow(2, D1)); break;
				case MT_KEYSIG:		printf("Key Signature       | "); PrintBuf(ptr); break;
				case MT_SEQMETA:	printf("Sequencer-Specific  | Length: %d", ptr->len); break;
				case MT_ENDOFTRK:	printf("End Of Track        | "); break;
				}
				ENDL;
				break;

			case ET_CTRLCHG:
				if (CT_SUSTPD   == D1
				||	CT_MODWHEEL == D1)
					break;

				printf("%8d | Ch%2d | ", ptr->tick, ptr->chn);
				switch (D1)
				{
				case CT_BANKSEL:	printf("Bank Select         | %d", D2); break;
				case CT_BANKSELL:	printf("Bank Select LSB     | %d", D2); break;
				case CT_CHNVOL:		printf("Channel Volume      | %d", D2); break;
				case CT_CHNPAN:		printf("Channel Pan         | ");
					if   (0x40 == D2) printf("C");
					else (D2 > 0x40)? printf("R%d", D2 - 0x40): printf("L%d", 0x40 - D2);
					break;

				case CT_MODWHEEL:	printf("Modulation wheel    | %d", D2); break;
				case CT_EXPRES:		printf("Expression          | %d", D2); break;
				case CT_SUSTPD:		printf("Sustain Pedal       | "); PrintSwt(D2); break;
				case CT_REVERB:		printf("Reverb Depth        | %d", D2); break;
				case CT_TREMELO:	printf("Tremelo Depth       | %d", D2); break;
				case CT_CHORUS:		printf("Chorus Depth        | %d", D2); break;
				case CT_DETUNE:		printf("Detune Depth        | %d", D2); break;
				case CT_PHASER:		printf("Phaser Depth        | %d", D2); break;

				case MM_RESETCTRL:	printf("Channel Mode Msg    | Reset All Controllers"); break;
				case MM_LOCALCTRL:	printf("Channel Mode Msg    | Local Control: "); PrintSwt(D2); break;
				case MM_SOUNDOFF:	printf("Channel Mode Msg    | All Sound Off"); break;
				case MM_NOTESOFF:	printf("Channel Mode Msg    | All Notes Off"); break;
				case MM_OMNIOFF:	printf("Channel Mode Msg    | Omni Mode Off"); break;
				case MM_OMNION:		printf("Channel Mode Msg    | Omni Mode On"); break;
				case MM_MONOON:		printf("Channel Mode Msg    | Mono Mode On"); break;
				case MM_POLYON:		printf("Channel Mode Msg    | Poly Mode On"); break;
				default:			printf("Controller: 0x%02X    | %d", D1, D2);
				}
				ENDL;
				goto FLAG;

			case ET_PROGCHG: printf("%8d | Ch%2d | Program Change      | %d\n", ptr->tick, ptr->chn, D1);
				goto FLAG;

			case ET_KEYPRES:
			case ET_CHNPRES:
			case ET_PITCHBD: mcnt++;
				goto FLAG;
#if 1
			case ET_NOTEON:  ncnt++;
			/* no break */
			case ET_NOTEOFF:
FLAG:			cflg |= 1 << ptr->chn;
				break;

#else
			case ET_NOTEON:  printf("%8d | Ch%2d | Note On             | %d\n", ptr->tick, ptr->chn, D1);
				ncnt++;
				goto FLAG;

			case ET_NOTEOFF: printf("%8d | Ch%2d | Note Off            | %d\n", ptr->tick, ptr->chn, D1);
FLAG:			cflg |= 1 << ptr->chn;
				break;
#endif
			} /* switch ptr->type */

		} /* for next event */

		printf("\nNote Count: %d", ncnt);
		printf("\nMisc Count: %d\n", mcnt);

		if (cflg)
		{
			printf("Channel Used: ");
			PrintChn(cflg); ENDL;
		}

	} /* for next track */

	printf("\nOverall Channel Used: ");
	PrintChn(aflg); ENDL;
}

void PrintFilteredEvent(const SMFEVT *evt)
{
	printf("  FILTERED: Track: %2d, Tick: %8d, ", evt->trk, evt->tick);

	if (FAIL == VeriEvt(evt))
	{
		printf("Invalid Event,  Data: ");
		PrintBuf(evt); ENDL;
		return;
	}

	switch (evt->type)
	{
	case ET_SYSEXF0:
	case ET_SYSEXF7:	printf("SysEx Event,    "); break;

	case ET_META:		printf("Meta Event: ");
		switch (evt->mtype)
		{
		case MT_TEXT:		printf("Text,         "); break;
		case MT_LYRIC:		printf("Lyric,        "); break;
		case MT_CUEPOINT:	printf("Cue Point,    "); break;
		case MT_SEQMETA:	printf("Seq-Specific, "); break;
		case MT_OFFSET:		printf("SMTPE Offset, "); break;
		case MT_SEQNUM:		printf("Seq Number,   "); break;
		case MT_COPYRIGHT:	printf("Copyright,    "); break;
		case MT_SETTEMPO:	printf("Set Tempo,    "); break;
		case MT_TIMESIG:	printf("Time Signat., "); break;
		case MT_KEYSIG:		printf("Key Signat.,  "); break;
		case MT_MARKER:		printf("Marker,       "); break;
		case MT_TRKNAME:	printf("Track Name,   "); break;
		case MT_INSTNAME:	printf("Inst. Name,   "); break;
		case MT_CHNFIX:		printf("Chn Prefix,   "); break;
		case MT_PORTFIX:	printf("Port Prefix,  "); break;
		case MT_ENDOFTRK:	printf("End of Track, "); break;
		}
		break;

	case ET_NOTEOFF:	printf("Note Off,       "); break;
	case ET_NOTEON:		printf("Note On,        "); break;
	case ET_KEYPRES:	printf("Key Aftertouch, "); break;
	case ET_CTRLCHG:	printf("Ctrl Change,    "); break;
	case ET_PROGCHG:	printf("Prog Change,    "); break;
	case ET_CHNPRES:	printf("Chn Aftertouch, "); break;
	case ET_PITCHBD:	printf("Pitch Bend,     "); break;
	}

	printf("Data: ");
	PrintBuf(evt); ENDL;
}

/****************************************************************/

void PrintBuf(const SMFEVT *evt)
{
	if (NULL == evt
	||	NULL == evt->data
	||	0 == evt->len)
		return;

	printf("0x");
	for (size_t i = 0; i < evt->len; i++)
		printf("%02X ", evt->data[i]);
}

void PrintStr(const SMFEVT *evt)
{
	if (NULL == evt
	||	NULL == evt->data
	||	0 == evt->len
	||	ET_META != evt->type)
		return;

	for (size_t i = 0; i < evt->len; i++)
		printf("%c", evt->data[i]);
}

void PrintVal(const SMFEVT *evt)
{
	if (NULL == evt
	||	NULL == evt->data
	||	0 == evt->len
	||	evt->len > LEN)
		return;

	printf("%d", ReadValE(evt));
}

void PrintSwt(BYTE val)
{
	if (val > MAX_VAL)
		return;

	(val > 0x3F)? printf("On"): printf("Off");
}

void PrintChn(WORD flg)
{
	for (BYTE c = 0; c <= MAX_CHN; c++)
		if (1 << c & flg)
			printf("%d, ", c);
}

void PrintSpr(char chr, size_t cnt)
{
	for (size_t i = 0; i < cnt; i++)
		printf("%c", chr);
}
