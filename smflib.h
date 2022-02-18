/*  smflib.h
    Created on: 2016-12-26
    Updated on: 2021-10-15
    Author: LyReonn
    GitHub: https://github.com/lyreonn
*/

#ifndef SMFLIB_H_
#define SMFLIB_H_

//  If defined, tracks without Track Name Meta Event will be filled with a blank one.
//  This will fix a bug on Apple platform which will not read MIDI channel correctly.
//#define _FILL_BLANK_TRACK_NAME

//  If defined, a GS Reset SysEx Event will be added at the start of the first track.
//  Necessary controllers with a default value will be written on every used channel.
//#define _RESET_ALL_CONTROLLERS

//  If defined, filtered events will be printed when calling WriteSMF().
//#define _PRINT_FILTERED_EVENTS

typedef char            CHAR;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;       // 2 bytes on mac
typedef unsigned int    DWORD;      // 4 bytes on mac
typedef unsigned long   QWORD;      // 8 bytes on mac
typedef size_t          SIZE;       // unsigned long on mac

#define HEADER_TOTAL_LENGTH (sizeof(DWORD) * 2 + HEADER_CHUNK_LENGTH)
#define HEADER_CHUNK_LENGTH (sizeof(WORD) * 3)
#define HEADER_CHUNK_TYPE   "MThd"
#define TRACK_CHUNK_TYPE    "MTrk"

#pragma region // typedef enum

typedef
    enum tagEventType
    {
    //  MIDI Event:
        EVENT_NOTEOFF   = 0x80,     // 0x8c key vel
        EVENT_NOTEON    = 0x90,     // 0x9c key vel
        EVENT_KEYPRS    = 0xA0,     // 0xAc key val
        EVENT_CONTROL   = 0xB0,     // 0xBc ctrl val
        EVENT_PROGRAM   = 0xC0,     // 0xCc prog
        EVENT_CHANPRS   = 0xD0,     // 0xDc val
        EVENT_BEND      = 0xE0,     // 0xEc lsb msb
    //  System Common Event:
        EVENT_TIMECODE  = 0xF1,     // 0xF1 type+val
        EVENT_SONGPOS   = 0xF2,     // 0xF2 lsb msb
        EVENT_SONGSEL   = 0xF3,     // 0xF3 song
        EVENT_TUNEREQ   = 0xF6,     // 0xF6
    /*  System Real-Time Event:
        EVENT_TIMING    = 0xF8,
        EVENT_START     = 0xFA,
        EVENT_CONTINUE  = 0xFB,
        EVENT_STOP      = 0xFC,
        EVENT_SENSING   = 0xFE,
        EVENT_RESET     = 0xFF, */
    //  System Exclusive Event:
        EVENT_SYSEXF0   = 0xF0,     // 0xF0 vlen data
        EVENT_SYSEXF7   = 0xF7,     // 0xF7 vlen data
    //  Meta Event:
        EVENT_META      = 0xFF,     // 0xFF type vlen data
    //  Beginning of the first track, if format 2, beginning of a track:
        META_OFFSET     = 0xFF54,   // 0xFF 0x54 0x05 hr mn se fr ff
        META_SEQNUM     = 0xFF00,   // 0xFF 0x00 0x02 nn nn
        META_COPYRIGHT  = 0xFF02,   // 0xFF 0x02 vlen text
    //  First track:
        META_TEMPO      = 0xFF51,   // 0xFF 0x51 0x03 tt tt tt
        META_TIMESNT    = 0xFF58,   // 0xFF 0x58 0x04 nn dd cc bb
        META_KEYSNT     = 0xFF59,   // 0xFF 0x59 0x02 sf mi
        META_MARKER     = 0xFF06,   // 0xFF 0x06 vlen text
    //  Beginning of a track:
        META_TRACKNAME  = 0xFF03,   // 0xFF 0x03 vlen text
        META_INSTNAME   = 0xFF04,   // 0xFF 0x04 vlen text
        META_CHANFIX    = 0xFF20,   // 0xFF 0x20 0x01 chan
        META_PORTFIX    = 0xFF21,   // 0xFF 0x21 0x01 port
    //  Anywhere:
        META_TEXT       = 0xFF01,   // 0xFF 0x01 vlen text
        META_LYRIC      = 0xFF05,   // 0xFF 0x05 vlen text
        META_CUEPOINT   = 0xFF07,   // 0xFF 0x07 vlen text
        META_SEQSPEC    = 0xFF7F,   // 0xFF 0x7F vlen data
    //  End of a track:
        META_ENDOFTRACK = 0xFF2F,   // 0xFF 0x2F 0x00
    }
TYPE;

typedef
    enum tagControlChange
    {
        CC_BANKSEL      = 0x00,     // Bank Select
        CC_MODULATION   = 0x01,     // Modulation Wheel
        CC_BREATH       = 0x02,     // Breath Control
        CC_CHANVOL      = 0x07,     // Channel Volume
        CC_CHANPAN      = 0x0A,     // Channel Pan
        CC_EXPRESSION   = 0x0B,     // Expression Controller
        CC_BANKLSB      = 0x20,     // Bank Select LSB
        CC_SUSTAIN      = 0x40,     // Sustain Pedal
    //  Channel Mode Message (Mode Change):
        MC_SOUNDOFF     = 0x78,     // All Sound Off
        MC_RESETCTRL    = 0x79,     // Reset All Controllers
        MC_LOCALCTRL    = 0x7A,     // Local Control On/Off
        MC_NOTESOFF     = 0x7B,     // All Notes Off
        MC_OMNIOFF      = 0x7C,     // Omni Mode Off
        MC_OMNION       = 0x7D,     // Omni Mode On
        MC_MONOON       = 0x7E,     // Mono Mode On
        MC_POLYON       = 0x7F      // Poly Mode On
    }
CTRL;

#pragma endregion // typedef enum

#pragma region // typedef struct

typedef
    struct tagEvent
    {
        DWORD tick;
         TYPE type;
         WORD track;
         SIZE length;
        const BYTE *buffer; // point to where this event start in the buffer (excluding tick)
        struct tagEvent *next;
    }
EVENT;

typedef
    struct tagStandardMidiFile
    {
         WORD format;
         WORD division;     // 2 forms: ticks per qnote or ticks per frame (using SMPTE time code)
         WORD trackCount;
        DWORD eventCount;
        EVENT *firstEvent;
         SIZE size;
         BYTE *buffer;
        const char *filePath;
    }
SMF;

#pragma endregion // typedef struct

#pragma region // typedef error code

typedef
    enum tagErrorCodeGeneral
    {
        GENERAL_SUCCESS = 0,
        GENERAL_INPUT_ERROR,
        GENERAL_MALLOC_ERROR,
    }
ERROR_CODE_GENERAL;

typedef
    enum tagErrorCodeReadSMF
    {
        READSMF_SUCCESS = 0,
        READSMF_CANNOT_OPEN_FILE = 10,
        READSMF_FILE_SIZE_ERROR,
        READSMF_SIZE_MISMATCH,
        READSMF_NOT_MIDI_FILE,
        READSMF_INVALID_MIDI_FILE,
    }
ERROR_CODE_READSMF;

typedef
    enum tagErrorCodePrintSMF
    {
        PRINTSMF_SUCCESS = 0,
        PRINTSMF_INVALID_MEMBER_VALUE = 20,
    }
ERROR_CODE_PRINTSMF;

#pragma endregion // typedef error code

//  calloc will initialize the members of struct to 0 or NULL:
#define NewSMF()    (SMF *)calloc(1, sizeof(SMF))
#define NewEvent()  (EVENT *)calloc(1, sizeof(EVENT))

int ReadSMF(const char *filePath, SMF *smf);
int PrintSMF(const SMF *smf);
void CloseSMF(SMF *smf);
//int WriteSMF(const SMF *smf, WORD format);

#endif /* SMFLIB_H_ */
