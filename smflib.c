
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smflib.h"

int main(const int argc, const char *argv[])
{
    if (4 != argc
    ||  0 != strcmp(argv[1], "-print")
    ||  0 != strcmp(argv[2], "-i"))
    {
        printf("usage: smflib -print -i input.mid\n");
        return GENERAL_INPUT_ERROR;
    }

    SMF *smf = NewSMF();
    int errorCode = ReadSMF(argv[3], smf);

    if (READSMF_SUCCESS != errorCode)
    {
        printf("error reading midi file! error code: %d\n", errorCode);
        //CloseSMF(smf);
        return errorCode;
    }

    printf("file size: %d\n", (int)(smf->size));
    //CloseSMF(smf);

    return GENERAL_SUCCESS;
}