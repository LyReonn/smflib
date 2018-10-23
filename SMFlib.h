/*
 * SMFlib.h
 *
 *	Created on: 2016-12-26
 *		Author: LyReonn
 *
 *	Abbreviations:
 *		lib = Library,	SMF = Standard MIDI File;
 *		var = Variable,	val = Value,	str = String,	arr = Array;
 *		buf = Buffer,	len = Length,	num = Number,	cnt = Count;
 *		evt = Event,	trk = Track,	chn = Channel,	seq = Sequence;
 *		stat = Status,	calc = Calculate, flg, msg, ptr, ord, beg
 *
 */

#ifndef SMFLIB_H_
#define SMFLIB_H_

//	If defined, tracks without Track Name Meta Event will be filled with a blank one.
//	This will fix a bug on Apple platform which will not read MIDI channel correctly.
#define _FILL_BLANK_TRACK_NAME

//	If defined, a GS Reset SysEx Event will be added at the start of the first track.
//	Necessary controllers with a default value will be written on every used channel.
#define _RESET_ALL_CONTROLLERS

//	If defined, filtered events will be printed when calling SMFWrite().
//	#define _PRINT_FILTERED_EVENTS

#define LEN		4

typedef size_t			SIZE;
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned int	DWRD;

typedef enum tagBoolean {
	FAIL	= 0,
	TRUE	= 1,
}	BOOL;

typedef enum tagEventType {
//	MIDI Event:
	ET_NOTEOFF		= 0x08,
	ET_NOTEON		= 0x09,
	ET_KEYPRES		= 0x0A,		// Polyphonic Key Pressure, aka Aftertouch
	ET_CTRLCHG		= 0x0B,		// Controller Change
	ET_PROGCHG		= 0x0C,		// Program Change
	ET_CHNPRES		= 0x0D,		// Channel Key Pressure
	ET_PITCHBD		= 0x0E,		// Pitch Bend
//	SysEx Event:
	ET_SYSEXF0		= 0xF0,		// System Exclusive
	ET_SYSEXF7		= 0xF7,
//	Meta Event:
	ET_META			= 0xFF
}	EVTTYPE;

typedef enum tagControllerType {
//	Channel Sound Setting:
	CT_BANKSEL		= 0x00,		// Bank Select
	CT_BANKSELL		= 0x20,		// Bank Select LSB
	CT_CHNVOL		= 0x07,		// Channel Volume
	CT_CHNPAN		= 0x0A,		// Channel Pan
//	Controller:
	CT_MODWHEEL		= 0x01,		// Modulation Wheel
	CT_EXPRES		= 0x0B,		// Expression Controller
	CT_SUSTPD		= 0x40,		// Sustain Pedal
	CT_SOSTPD		= 0x42,		// Sostenuto Pedal
	CT_SOFTPD		= 0x43,		// Soft Pedal
//	Effect Depth:
	CT_REVERB		= 0x5B,		// External Effects Depth
	CT_TREMELO		= 0x5C,		// Tremelo Depth
	CT_CHORUS		= 0x5D,		// Chorus Depth
	CT_DETUNE		= 0x5E,		// Celeste(Detune) Depth
	CT_PHASER		= 0x5F,		// Phaser Depth
//	Channel Mode Message:
	MM_RESETCTRL	= 0x79,		// Reset All Controllers
	MM_LOCALCTRL	= 0x7A,		// Local Control On/Off
	MM_SOUNDOFF		= 0x78,		// All Sound Off
	MM_NOTESOFF		= 0x7B,		// All Notes Off
	MM_OMNIOFF		= 0x7C,		// Omni Mode Off
	MM_OMNION		= 0x7D,		// Omni Mode On
	MM_MONOON		= 0x7E,		// Mono Mode On
	MM_POLYON		= 0x7F		// Poly Mode On
}	CTRLTYPE;

typedef enum tagMetaType {
//	EF_TRK1:
	MT_SETTEMPO		= 0x51,		// Set Tempo
	MT_TIMESIG		= 0x58,		// Time Signature
	MT_KEYSIG		= 0x59,		// Key Signature
	MT_MARKER		= 0x06,
//	EF_TRK1BEG:
	MT_OFFSET		= 0x54,		// SMTPE Offset
	MT_SEQNUM		= 0x00,		// Sequence Number
	MT_COPYRIGHT	= 0x02,		// Copyright Notice
//	EF_TRKBEG:
	MT_TRKNAME		= 0x03,		// Sequence/Track Name
	MT_INSTNAME		= 0x04,		// Instrument Name
	MT_CHNFIX		= 0x20,		// MIDI Channel Prefix
	MT_PORTFIX		= 0x21,		// MIDI Port Prefix
//	EF_TRKEND:
	MT_ENDOFTRK		= 0x2F,		// End of Track
//	EF_DISCARD:
	MT_TEXT			= 0x01,		// Text Event
	MT_LYRIC		= 0x05,
	MT_CUEPOINT		= 0x07,		// Cue Point
	MT_SEQMETA		= 0x7F		// Sequencer-Specific Meta-event
}	METATYPE;

typedef struct tagSMFEvent {
	EVTTYPE  type; union {
	METATYPE mtype;
	BYTE     chn;        };
	WORD     trk;  union {
	DWRD     time;				// Absolute Time Stamp (in Microseconds)
	DWRD     tick;       };		// Absolute Ticks = time / settempo * div
	SIZE     len;
	BYTE    *data;
}	SMFEVT;

typedef struct tagSMFFile {
	BOOL     fmt;				// File Format
	WORD     div;				// Division
	WORD     tcnt;				// Track Count
	DWRD     ecnt;				// Event Count
	SIZE     size;				// File Size
	BYTE    *buf;				// File Buffer
	SMFEVT  *evt;				// Pointer to First Event
}	SMFILE;

SMFILE *SMFRead(const char *filename);
SMFILE *SMFWrite(const SMFILE *smf, BOOL fmt);

void SMFPrint(const SMFILE *smf);
void SMFClose(SMFILE *smf);

#endif /* SMFLIB_H_ */
