/*
 * SMFlib_general.h
 *
 *  Created on: 2016-12-26
 *      Author: LyReonn
 */

#ifndef SMFLIB_GENERAL_H_
#define SMFLIB_GENERAL_H_

/*****************************DEFAULT****************************/

#define MAX_CHN			15				// 0 ~ 15
#define MAX_TRK			32				// 1 ~ 32
#define MAX_VAL			0x7F			// 0 ~ 127

#define DV_DIVISION		0x01E0			// 480
#define DV_SETTEMPO		0x07A120		// 500000
#define DV_TIMESIG		0x04021808		// 4/4

#define DV_TRKNAME		0x00FF0300
#define DV_ENDOFTRK		0x00FF2F00

static BYTE TIME_SIG[4] = {
	0x04, 0x02, 0x18, 0x08
};

static BYTE GS_RESET[13] = {
	0x00, 0xF0, 0x0A,
	0x41, 0x00, 0x42, 0x12,
	0x40, 0x00, 0x7F, 0x00,
	0x41, 0xF7
};

//	Used BYTEs: 4 + 4 + 3 * 12 = 44
#define RESET_CTRL(chn)	((0x00B0 | chn) << 16 | 0x7900)
#define DV_PITCHBD(chn)	((0x00E0 | chn) << 16 | 0x0040)

#define DV_BANKSEL		0x000000
#define DV_BKSELLSB		0x002000
#define DV_MODWHEEL		0x000100
#define DV_CHNVOL		0x000764
#define DV_CHNPAN		0x000A40
#define DV_EXPRESS		0x000B7F

#define DV_SUSTPD		0x004000
#define DV_PORTA		0x004100
#define DV_SOSTPD		0x004200
#define DV_SOFTPD		0x004300
#define DV_REVERB		0x005B00
#define DV_CHORUS		0x005D00

void WriteDefaultVal(BYTE **buf, BYTE chn);

/*****************************BUFFER*****************************/

SIZE GetMsgLen(BYTE statusByte);
SIZE GetValLen(DWRD varLenVal);
void SkipBuf(const BYTE **buf, SIZE cnt);

BYTE ReadBuf(const BYTE **buf);
DWRD ReadVal(const BYTE **buf, SIZE cnt);
DWRD ReadValS(const BYTE *buf, SIZE cnt);
DWRD ReadValE(const SMFEVT *evt);
DWRD ReadVarLenVal(const BYTE **buf);
DWRD ReadVarLenValS(const BYTE *buf);
BYTE *ReadArr(const BYTE **buf, SIZE cnt);
BYTE *ReadArrS(const BYTE *buf, SIZE cnt);
const char *ReadStr(const BYTE **buf, SIZE cnt);
const char *ReadStrS(const BYTE *buf, SIZE cnt);

void WriteBuf(BYTE **buf, BYTE val);
void WriteVal(BYTE **buf, DWRD val, SIZE cnt);
void WriteValS(BYTE *buf, DWRD val, SIZE cnt);
void WriteVarLenVal(BYTE **buf, DWRD val);
void WriteVarLenValS(BYTE *buf, DWRD val);
void WriteArr(BYTE **buf, const BYTE *arr, SIZE cnt);
void WriteArrS(BYTE *buf, const BYTE *arr, SIZE cnt);
void WriteStr(BYTE **buf, const char *str);
void WriteStrS(BYTE *buf, const char *str);

/*****************************VERIFY*****************************/

#define VERI_SUCCESS			 0
#define VERI_INVALID_POINTER	-1
#define VERI_INVALID_FIELD		-2
#define VERI_INVALID_DATA		-3

#define VERIBUF_INVALID_BUFFER	-4
#define VERIBUF_SIZE_MISMATCH	-5
//	File format can only be 0 or 1;
BOOL VeriBuf(const SMFILE *smf);

#define VERIEVT_DATA_MISMATCH	-4
BOOL VeriEvt(const SMFEVT *evt);

#define VERISMF_INVALID_EVENT	-4
#define VERISMF_TRACK_EXCEED	-5
//	Verify every evt's validity;
BOOL VeriSmf(const SMFILE *smf);

#define VERIBUFSMF_MISMATCH		-4
//	Include VeriBuf;
//	Verify if fmt, tcnt, div and ecnt match buf:
BOOL VeriBufSmf(const SMFILE *smf);

#define VERIORD_TICK_OUT_OF_ORDER		-4
#define VERIORD_TRACK_OUT_OF_ORDER		-5
#define VERIORD_TRACK_DISCONTINUITY		-6
#define VERIORD_TRACK_COUNT_MISMATCH	-7
BOOL VeriEvtOrd(const SMFILE *smf);

/*****************************FILTER*****************************/

typedef enum tagEventFilter {
	EF_DISCARD	= 0,
	EF_KEEP		= 1,
	EF_TRK1,
	EF_TRK1BEG,
	EF_TRKBEG,
	EF_TRKEND,
}	eEVTFILTER;

typedef struct tagMetaStatus {
	SMFEVT *Offset;
	SMFEVT *SeqNum;
	SMFEVT *TimeSig;		// First Time Signature Event Pointer
	SMFEVT *SetTempo;		// First Set Tempo Event Pointer
	DWRD stcnt;				// Set Tempo Event Count
	DWRD tscnt;				// Time Signature Event Count
	DWRD mkcnt;				// Marker Event Count
	DWRD stval;				// Last Set Tempo Value
	DWRD tsval;				// Last Time Signature Value
}	METASTAT;

typedef struct tagTrackStatus {
	SMFEVT *TrkName;
	SMFEVT *EndOfTrk;
	DWRD mcnt;				// MIDI Event Count
	DWRD ncnt;				// Note Count
	WORD cflg;				// Channel Flag
}	TRKSTAT;

typedef struct tagChannelStatus {
	BYTE pcval;				// Last Program Change Value
	BYTE ccval[MAX_VAL+1];	// Last Controller Change Value
	BOOL nstat[MAX_VAL+1];	// Note Status (Currently On or Off)
}	CHNSTAT;

eEVTFILTER EventFilter(const SMFEVT *evt);

void ResetFilterLastVal(void);		// Clear last values and note status for following process.
void ResetFilterStat(void);			// Clear event count and pointer, then set status writable.
void LockFilterStat(void);			// Set status read-only.
BOOL GetFilterStat(void);			// Returns TRUE when writable, FAIL when read-only.

const SMFEVT *GetOffset(void);
const SMFEVT *GetSeqNum(void);
const SMFEVT *GetFirstSetTempo(void);
const SMFEVT *GetFirstTimeSig(void);
DWRD GetSetTempoCnt(void);
DWRD GetTimeSigCnt(void);
DWRD GetMarkerCnt(void);
DWRD GetLastSetTempoVal(void);
DWRD GetLastTimeSigVal(void);

#define INVALID		0xFF			// Invalid value for chn and data.
const SMFEVT *GetTrkName(WORD trk);
const SMFEVT *GetEndOfTrk(WORD trk);
DWRD GetMidiEvtCnt(WORD trk);
DWRD GetNoteCnt(WORD trk);
WORD GetChnFlg(WORD trk);			// Returns 0 ~ 0xFFFF;
BYTE GetChnCnt(WORD trk);			// Returns 0 ~ 16;
BYTE GetChnNum(WORD trk);			// Returns 0 ~ 15  or INVALID:

WORD GetOverallChnFlg(void);
BYTE GetOverallChnCnt(void);
BYTE GetCurOnNoteCnt(BYTE chn);		// Returns 0 ~ 127 or INVALID:

/*****************************PRINT******************************/

void PrintFilteredEvent(const SMFEVT *evt);

void PrintBuf(const SMFEVT *evt);
void PrintStr(const SMFEVT *evt);
void PrintVal(const SMFEVT *evt);
void PrintSwt(BYTE val);
void PrintChn(WORD flg);
void PrintSpr(char chr, SIZE cnt);

#define PrintSPR PrintSpr('>', 64);printf("\n")

/****************************************************************/

#endif /* SMFLIB_GENERAL_H_ */
