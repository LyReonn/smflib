/*
 * main.c
 *
 *  Created on: 2016-12-28
 *      Author: LyReonn
 */

#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#include "SMFlib.h"
#include "SMFlib_general.h"
#include "Module/waterfall.h"
#include "Module/baroffset.h"

int main(void)
{
	struct _finddata_t file;
	long handle;
	DWRD scnt = 0, fcnt = 0;
	int  evtcnt = 0, bytecnt = 0;

	handle = _findfirst("*.mid", &file);

	if (-1 == handle)
	{
		printf("ERROR: MIDI FILE NOT FOUND!\n");
		system("pause");
		return(-1);
	}

	do
	{
		SMFILE *smf  = NULL;
		SMFILE *out  = NULL;
		FILE *stream = NULL;

		printf("PROCESSING: \"%s\"...\n", file.name);
		smf = SMFRead(file.name);

		if (NULL == smf)
		{
			printf("ERROR: READING FAILED!\n");
FAIL:		PrintSPR;
			fcnt++;

			SMFClose(smf);
//			SMFClose(out);
			fclose(stream);

			continue;
		}

//		SMFPrint(smf);

#if 1
		out = SMFWrite(smf, 1);

		if (NULL == out)
		{
			printf("ERROR: WRITING BUFFER FAILED!\n");
			goto FAIL;
		}

//		SMFPrint(out);

		stream = fopen(file.name, "wb");

		if (NULL == stream
		||	fwrite(out->buf, 1, out->size, stream) != out->size)
		{
			printf("ERROR: WRITING FILE FAILED!\n");
			goto FAIL;
		}
#endif

#if 0
		switch (WriteWaterfall(file.name, smf))
		{
		case WATERFALL_FILTER_FAILED:  printf("ERROR: FILTERING FAILED!\n"); goto FAIL;
		case WATERFALL_MULTIPLE_TEMPO: printf("ERROR: MULTIPLE TEMPOS!\n"); goto FAIL;
		case WATERFALL_TOO_MANY_CHN:   printf("ERROR: MORE THAN TWO CHANNELS DETECTED!\n"); goto FAIL;
		case WATERFALL_TOO_MANY_TRK:   printf("ERROR: MORE THAN TWO TRACKS CONTAIN MIDI EVENTS!\n"); goto FAIL;
		case WATERFALL_ONLY_ONE_CHN:   printf("ERROR: ONLY ONE CHANNEL AND ONLY ONE TRACK!\n"); goto FAIL;
		case WATERFALL_CHN_INCORRECT:  printf("ERROR: CHANNEL NUMBER INCORRECT! (CORRECT NUMBER: 0 or/and 1)\n"); goto FAIL;
		case WATERFALL_WRITE_FAILED:   printf("ERROR: WRITING FILE FAILED!\n"); goto FAIL;
		case WATERFALL_SUCCESS: break;
		}
#endif

#if 0
		switch (BarOffset(smf))
		{
		case BAROFFSET_FILTER_FAILED: printf("ERROR: FILTERING FAILED!\n"); goto FAIL;
		case BAROFFSET_TOO_MANY_CHN:  printf("ERROR: MORE THAN TWO CHANNELS DETECTED!\n"); goto FAIL;
		case BAROFFSET_UPBEAT:        printf("ERROR: UPBEAT! NO NEED TO PROCESS!\n"); goto FAIL;
		case WATERFALL_SUCCESS: break;
		}

		extern BOOL CalcFileSize(SMFILE *);
		extern BOOL WriteFileBuf(SMFILE *);

		if (FAIL == CalcFileSize(smf)
		||	FAIL == WriteFileBuf(smf))
		{
			printf("ERROR: WRITING BUFFER FAILED!\n");
			goto FAIL;
		}

		stream = fopen(file.name, "wb");

		if (NULL == stream
		||	fwrite(smf->buf, 1, smf->size, stream) != smf->size)
		{
			printf("ERROR: WRITING FILE FAILED!\n");
			goto FAIL;
		}
#endif

		printf("SUCCESS! %d EVENTS FILTERED, SAVED %d BYTES.\n", smf->ecnt - out->ecnt, smf->size - out->size);
		PrintSPR;

		scnt++;
		evtcnt  += smf->ecnt - out->ecnt;
		bytecnt += smf->size - out->size;

		SMFClose(smf);
//		SMFClose(out);
		fclose(stream);

	} while (!_findnext(handle, &file));

	_findclose(handle);

	printf("COMPLETED! %d SUCCESS, %d FAILURE.\n", scnt, fcnt);
	printf("OVERALL FILTERED %d EVENTS, SAVED %d BYTES.\n", evtcnt, bytecnt);
	system("pause");

	return 0;
}
