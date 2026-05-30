#include "digitizeroperation.h"
#include <format>

DigitizerOperation::DigitizerOperation() {}

DigitizerOperation::DigitizerOperation(CAEN_DGTZ_ErrorCode dtz) {
    this->ret = dtz;
}

DigitizerOperation::~DigitizerOperation() {
    qDebug() << "Digitizer finished" ;
}

int DigitizerOperation::setErrorCode(CAEN_DGTZ_ErrorCode dtz){
    try {
        this->ret = dtz;
        if (this->ret != CAEN_DGTZ_Success)
            throw "Error" + std::to_string(this->ret) + "Check your digitizer!";
    }catch(const char* error_message){
        qDebug() << error_message;
    }
    return 0;
}

CAEN_DGTZ_ErrorCode DigitizerOperation::getErrorCode(){
    return this->ret;
}

CAEN_DGTZ_BoardInfo_t DigitizerOperation::getBoardInfo(){
    return this->BoardInfo;
}
CAEN_DGTZ_EventInfo_t DigitizerOperation::getEventInfo(){
    return this->eventInfo;
}
