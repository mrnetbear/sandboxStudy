#ifndef DIGITIZEROPERATION_H
#define DIGITIZEROPERATION_H

#include "CAENDigitizer.h"

#include <iostream>

#include <QDebug>

#define MAXNB 1

class DigitizerOperation
{
private:
    CAEN_DGTZ_ErrorCode ret;
    CAEN_DGTZ_BoardInfo_t BoardInfo;
    CAEN_DGTZ_EventInfo_t eventInfo;
    CAEN_DGTZ_UINT16_EVENT_t *Evt = NULL;
    char *buffer = NULL;
    int MajorNumber;
    int i= sizeof(CAEN_DGTZ_TriggerMode_t);
    int c = 0, count[MAXNB];
    char * evtptr = NULL;
    uint32_t size,bsize;
    uint32_t numEvents;
public:
    int	handle[MAXNB];
    DigitizerOperation();
    DigitizerOperation(CAEN_DGTZ_ErrorCode);
    ~DigitizerOperation();
    CAEN_DGTZ_ErrorCode getErrorCode();
    int setErrorCode(CAEN_DGTZ_ErrorCode);
    CAEN_DGTZ_BoardInfo_t getBoardInfo();
    CAEN_DGTZ_EventInfo_t getEventInfo();
    int setBuffer();


};

#endif // DIGITIZEROPERATION_H
