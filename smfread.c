
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smflib.h"

#pragma region // function declaration

//  ReadSomething() moves the pointer forward, while GetSomething() doesn't.
//  We assume that variable length values are not larger than 0x0FFFFFFF.
BYTE  ReadByte(const BYTE **ptr);
CHAR *ReadString(const BYTE **ptr, const SIZE length);
DWORD ReadValueBigEndian(const BYTE **ptr, const SIZE length);
DWORD ReadValueVarLength(const BYTE **ptr);
CHAR *GetString(const BYTE *ptr, const SIZE length);
DWORD GetValueBigEndian(const BYTE *ptr, const SIZE length);
DWORD GetValueVarLength(const BYTE *ptr);
SIZE  GetLengthOfVarLenVal(const BYTE *ptr);

int ReadBuffer(const char *filePath, SMF *smf);
int ReadEvents(SMF *smf);
int SetTypeAndLength(EVENT *event);

#pragma endregion // function declaration

#pragma region // main functions

int ReadSMF(const char *filePath, SMF *smf)
{
    int errorCode = ReadBuffer(filePath, smf);

    if (READSMF_SUCCESS != errorCode)
    {
        return errorCode;
    }

    errorCode = ReadEvents(smf);

    if (READSMF_SUCCESS != errorCode)
    {
        return errorCode;
    }

    return READSMF_SUCCESS;
}

int ReadBuffer(const char *filePath, SMF *smf)
{
    FILE *fileStream = NULL;

    if (NULL == smf)
    {
        return GENERAL_INPUT_ERROR;
    }

    fileStream = fopen(filePath, "rb");

    if (NULL == fileStream)
    {
        return READSMF_CANNOT_OPEN_FILE;
    }

    if (0 != fseek(fileStream, 0, SEEK_END))
    {
        fclose(fileStream);
        return READSMF_FILE_SIZE_ERROR;
    }

    smf->size = ftell(fileStream);
    rewind(fileStream);

    if (-1 == smf->size
    ||   0 == smf->size)
    {
        fclose(fileStream);
        return READSMF_FILE_SIZE_ERROR;
    }

    smf->buffer = (BYTE *)calloc(smf->size, sizeof(BYTE));

    if (NULL == smf->buffer)
    {
        fclose(fileStream);
        return GENERAL_MALLOC_ERROR;
    }

    if (fread(smf->buffer, 1, smf->size, fileStream) != smf->size)
    {
        fclose(fileStream);
        return READSMF_SIZE_MISMATCH;
    }

    fclose(fileStream);

    return READSMF_SUCCESS;
}

int ReadEvents(SMF *smf)
{
    const BYTE *ptr[1] = {NULL};
    const BYTE *trackBegin = NULL;

    char *chunkType = NULL;
    DWORD chunkLength = 0;
    SIZE  totalLength = 0;

    DWORD tick = 0;
    EVENT *currentEvent = NULL;

    if (NULL == smf
    ||  NULL == smf->buffer
    ||     0 == smf->size)
    {
        return GENERAL_INPUT_ERROR;
    }

    if (smf->size <= HEADER_TOTAL_LENGTH)
    {
        return READSMF_NOT_MIDI_FILE;
    }

    #pragma region // read and verify header chunk

    ptr[0] = smf->buffer;
    chunkType = ReadString(ptr, sizeof(DWORD));
    totalLength += sizeof(DWORD);

    if (0 != strcmp(chunkType, HEADER_TYPE))
    {
        free(chunkType);
        return READSMF_NOT_MIDI_FILE;
    }

    free(chunkType);

    chunkLength = ReadValueBigEndian(ptr, sizeof(DWORD));
    totalLength += sizeof(DWORD);
    totalLength += chunkLength;

    if (chunkLength < HEADER_CHUNK_LENGTH)
    {
        return READSMF_INVALID_MIDI_FILE;
    }

    smf->format = ReadValueBigEndian(ptr, sizeof(WORD));

    if (smf->format > 2)
    {
        printf("warning! midi format is %d.\n", smf->format);
    }

    smf->trackCount = ReadValueBigEndian(ptr, sizeof(WORD));

    if (0 == smf->trackCount
    || (0 == smf->format
    &&  1 != smf->trackCount))
    {
        return READSMF_INVALID_MIDI_FILE;
    }

    smf->division = ReadValueBigEndian(ptr, sizeof(WORD));

    if (0 == smf->division)
    {
        return READSMF_INVALID_MIDI_FILE;
    }

    if (chunkLength > HEADER_CHUNK_LENGTH)
    {
        printf("warning! header chunk has %d bytes!\n", chunkLength);
        *ptr += chunkLength - HEADER_CHUNK_LENGTH;
    }

    #pragma endregion // read and verify header chunk

    #pragma region // read events in track chunk(s)

    smf->eventCount = 0;
    smf->firstEvent = NewEvent();
    currentEvent = smf->firstEvent;

    for (WORD track = 1; track <= smf->trackCount; track++, tick = 0)
    {
        chunkType = ReadString(ptr, sizeof(DWORD));
        chunkLength = ReadValueBigEndian(ptr, sizeof(DWORD));
        totalLength += sizeof(DWORD) * 2;
        totalLength += chunkLength;

        if (0 != strcmp(chunkType, TRACK_TYPE))
        {
            printf("warning! chunk %d has a type of %s, will be skipped!\n", track, chunkType);
            free(chunkType);
            *ptr += chunkLength;
            continue; // continue to next chunk
        }

        free(chunkType);
        trackBegin = *ptr;

        while (*ptr - trackBegin < chunkLength)
        {
            tick += ReadValueVarLength(ptr);
            currentEvent->buffer = *ptr;

            currentEvent->tick = tick;
            currentEvent->track = track;

            if (READSMF_SUCCESS != SetTypeAndLength(currentEvent))
            {
                return READSMF_INVALID_MIDI_FILE;
            }

            *ptr += currentEvent->length;
            smf->eventCount++;

            if (META_ENDOFTRACK == currentEvent->type)
            {
                if (*ptr - trackBegin != chunkLength)
                {
                    return READSMF_INVALID_MIDI_FILE;
                }

                if (track == smf->trackCount)
                {
                    currentEvent->next = NULL;
                    break;
                }
            }

            currentEvent->next = NewEvent();
            currentEvent = currentEvent->next;
        } // while loop, within a track
    } // for loop

    if (totalLength != smf->size)
    {
        return READSMF_SIZE_MISMATCH;
    }

    #pragma endregion // read events in track chunk(s)

    return READSMF_SUCCESS;
}

int SetTypeAndLength(EVENT *event)
{
    static BYTE runningStatus = 0;

    if (NULL == event
    ||  NULL == event->buffer)
    {
        return GENERAL_INPUT_ERROR;
    }

    if (event->buffer[0] < 0x80) // running status
    {
        if (runningStatus < 0x80
        ||  runningStatus >= 0xF0)
        {
            return READSMF_INVALID_MIDI_FILE;
        }

        event->type = runningStatus & 0b11110000;
        
        switch (event->type)
        {
        case EVENT_NOTEOFF:
        case EVENT_NOTEON:
        case EVENT_KEYPRS:
        case EVENT_CONTROL:
        case EVENT_BEND:
            event->length = 2;
            break;
        case EVENT_PROGRAM:
        case EVENT_CHANPRS:
            event->length = 1;
            break;
        default:
            return READSMF_INVALID_MIDI_FILE;
        }
    }
    else if (event->buffer[0] < 0xF0) // MIDI Events
    {
        runningStatus = event->buffer[0];
        event->type = runningStatus & 0b11110000;

        switch (event->type)
        {
        case EVENT_NOTEOFF:
        case EVENT_NOTEON:
        case EVENT_KEYPRS:
        case EVENT_CONTROL:
        case EVENT_BEND:
            event->length = 3;
            break;
        case EVENT_PROGRAM:
        case EVENT_CHANPRS:
            event->length = 2;
            break;
        default:
            return READSMF_INVALID_MIDI_FILE;
        }
    }
    else // SysEx and Meta Events
    {
        runningStatus = 0; // cancel running status

        if (EVENT_META == event->buffer[0])
        {
            event->type = (EVENT_META << 8) + event->buffer[1];
            event->length = 2 + GetLengthOfVarLenVal(event->buffer + 2) + GetValueVarLength(event->buffer + 2);
        }
        else
        {
            event->type = event->buffer[0];

            switch (event->type)
            {
            case EVENT_SYSEXF0:
            case EVENT_SYSEXF7:
                event->length = 1 + GetLengthOfVarLenVal(event->buffer + 1) + GetValueVarLength(event->buffer + 1);
                break;
            case EVENT_TIMECODE:
            case EVENT_SONGSEL:
                event->length = 2;
            case EVENT_SONGPOS:
                event->length = 3;
            case EVENT_TUNEREQ:
                event->length = 1;
            default:
                return READSMF_INVALID_MIDI_FILE;
            }
        }
    } // SysEx and Meta Events

    return READSMF_SUCCESS;
}

#pragma endregion // main functions

#pragma region // basic functions

BYTE ReadByte(const BYTE **ptr)
{
    if (NULL == ptr
    ||  NULL == *ptr)
    {
        return 0;
    }

    return (*(*ptr)++); // returns **ptr (BYTE) and *ptr (BYTE *) moves 1 byte forward
}

CHAR *ReadString(const BYTE **ptr, const SIZE length)
{
    char *bytes = NULL;

    if (NULL == ptr
    ||  NULL == *ptr
    ||     0 == length)
    {
        return NULL;
    }

    bytes = (char *)calloc(length + 1, sizeof(char));

    if (NULL == bytes)
    {
        return NULL;
    }

    for (SIZE i = 0; i < length; i++)
    {
        bytes[i] = ReadByte(ptr);
    }

    bytes[length] = '\0';

    return bytes;
}

CHAR *GetString(const BYTE *ptr, const SIZE length)
{
    return ReadString(&ptr, length);
}

DWORD ReadValueBigEndian(const BYTE **ptr, const SIZE length)
{
    DWORD value = 0;

    if (NULL == ptr
    ||  NULL == *ptr
    ||     0 == length
    ||  length > sizeof(DWORD))
    {
        return 0;
    }

    for (SIZE i = 0; i < length; i++)
    {
        value <<= 8;
        value += ReadByte(ptr);
    }

    return value;
}

DWORD GetValueBigEndian(const BYTE *ptr, const SIZE length)
{
    return ReadValueBigEndian(&ptr, length);
}

DWORD ReadValueVarLength(const BYTE **ptr)
{
    DWORD value = 0;

    if (NULL == ptr
    ||  NULL == *ptr)
    {
        return 0;
    }

    while (**ptr & 0b10000000)
    {
        value += ReadByte(ptr) & 0b01111111;
        value <<= 7;
    }

    value += ReadByte(ptr);

    return value;
}

DWORD GetValueVarLength(const BYTE *ptr)
{
    return ReadValueVarLength(&ptr);
}

SIZE GetLengthOfVarLenVal(const BYTE *ptr)
{
    DWORD length = 0;

    if (NULL == ptr)
    {
        return 0;
    }

    while (*ptr & 0b10000000)
    {
        length++;
        ptr++;
    }

    length++;

    return length;
}

#pragma endregion // basic functions
