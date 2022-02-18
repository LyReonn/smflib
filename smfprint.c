
#include <stdio.h>
#include <math.h>
#include "smflib.h"

#define ENDLINE printf("\n")
#define BUFFER  currentEvent->buffer

char *PrintKeySignature(BYTE sharpFlats, BYTE majorMinor);

extern CHAR *GetString(const BYTE *ptr, const SIZE length);
extern DWORD GetValueBigEndian(const BYTE *ptr, const SIZE length);
extern SIZE  GetLengthOfVarLenVal(const BYTE *ptr);

int PrintSMF(const SMF *smf)
{
    EVENT *currentEvent = NULL;
    DWORD eventCount = 0;
    DWORD noteCount = 0;
    WORD chanUsed = 0;
    WORD chanUsedAll = 0;

    if (NULL == smf
    ||  NULL == smf->buffer
    ||  NULL == smf->filePath
    ||  NULL == smf->firstEvent
    ||  0 == smf->size
    ||  0 == smf->eventCount)
    {
        return GENERAL_INPUT_ERROR;
    }

    if (0 == smf->trackCount
    ||  0 == smf->division)
    {
        return PRINTSMF_INVALID_MEMBER_VALUE;
    }

    printf("****************************************************************\n");
    printf("* File Path:   %s\n", smf->filePath);
    printf("* File Size:   %d\n", (int)smf->size);
    printf("****************************************************************\n");
    printf("* Format:      %d\n", smf->format);
    printf("* Track Count: %d\n", smf->trackCount);
    printf("* Event Count: %d\n", smf->eventCount);
    if (smf->division & 0x8000) {
    /* SMPTE Time Code */ }
    else {
    printf("* Division:    %d ticks per quarter-note\n", smf->division); }
    printf("****************************************************************\n");

    for (WORD track = 1; track <= smf->trackCount; track++, chanUsed = 0, noteCount = 0)
    {
        printf("* Track: %d\n", track);
        printf("* ____Tick|Chan|Event_______________|Value______________________\n");

        currentEvent = smf->firstEvent;
        
        while (NULL != currentEvent)
        {
            if (currentEvent->track == track)
            {
                printf("* %8d|", currentEvent->tick);

                switch (currentEvent->type)
                {
                case META_ENDOFTRACK: printf("Meta|End Of Track        |\n"); break;
                case META_TRACKNAME:  printf("Meta|Track Name          |%s\n", BUFFER + 2 + GetLengthOfVarLenVal(BUFFER + 2)); break;
                case META_INSTNAME:   printf("Meta|Instrument Name     |%s\n", BUFFER + 2 + GetLengthOfVarLenVal(BUFFER + 2)); break;
                case META_TIMESNT:    printf("Meta|Time Signature      |%d/%d\n", BUFFER[3], (int)pow(2, BUFFER[4])); break;
                case META_KEYSNT:     printf("Meta|Key Signature       |%s\n", PrintKeySignature(BUFFER[3], BUFFER[4])); break; 
                case META_TEMPO:      printf("Meta|Set Tempo           |%3.3f\n", 6E7 / GetValueBigEndian(BUFFER + 3, 3)); break;
                case META_OFFSET:     printf("Meta|SMPTE Offset        |\n"); break;
                case META_CHANFIX:    printf("Meta|Channel Prefix      |\n"); break;
                case EVENT_NOTEON:     printf("%4d|Note On             |\n", BUFFER[0] & 0b1111); break;
                case EVENT_NOTEOFF:    printf("%4d|Note Off            |\n", BUFFER[0] & 0b1111); break;
                case EVENT_CONTROL:    printf("%4d|Control Change      |\n", BUFFER[0] & 0b1111); break;
                case EVENT_PROGRAM:    printf("%4d|Program Change      |\n", BUFFER[0] & 0b1111); break;
                default: ENDLINE; break;
                }
            }

            currentEvent = currentEvent->next;
        }

        printf("****************************************************************\n");
    }



    return PRINTSMF_SUCCESS;
}

char *PrintKeySignature(BYTE sharpFlats, BYTE majorMinor)
{
    static char key[4] = {'C', '#', 'm', '\0'};
    return key;
}