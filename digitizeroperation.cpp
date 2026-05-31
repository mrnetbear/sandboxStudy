#include "digitizeroperation.h"

DigitizerOperation::DigitizerOperation(QObject *parent)
    : QObject(parent)
    , isAcquiring(false)
    , isOpened_flag(false)
    , isConfigured_flag(false)
    , acquisitionThread(nullptr)
{
    memset(handle, 0, sizeof(handle));
    memset(count, 0, sizeof(count));
    buffer = nullptr;
    Evt = nullptr;
    evtptr = nullptr;
}

DigitizerOperation::~DigitizerOperation()
{
    if (isAcquiring) {
        stopAcquisition();
    }
    if (isOpened_flag) {
        closeDigitizer();
    }
    if (buffer) {
        CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    }
    qDebug() << "Digitizer operation finished";
}

QString DigitizerOperation::errorCodeToString(CAEN_DGTZ_ErrorCode code)
{
    switch(code) {
    case CAEN_DGTZ_Success:
        return "Success";
    case CAEN_DGTZ_Timeout:
        return "Timeout";
    case CAEN_DGTZ_EventNotFound:
        return "Device not found";
    case CAEN_DGTZ_InvalidHandle:
        return "Invalid handle";
    case CAEN_DGTZ_InvalidParam:
        return "Invalid parameter";
    case CAEN_DGTZ_CalibrationError:
        return "Communication error";
    case CAEN_DGTZ_UnsupportedTrace:
        return "Unsupported feature";
    case CAEN_DGTZ_MaxDevicesError:
        return "Memory error";
    default:
        return QString("Unknown error code: %1").arg(code);
    }
}

CAEN_DGTZ_ErrorCode DigitizerOperation::getLastError()
{
    return ret;
}

QString DigitizerOperation::getLastErrorMessage()
{
    return lastErrorMessage;
}

bool DigitizerOperation::openDigitizer()
{
    emit progressUpdated("Opening digitizer...");

    for(int b = 0; b < MAXNB; b++) {
        // Открываем соединение с оцифровщиком через USB
        for (int i = 0; i < 256; ++i){
            ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, i, 0, 0, &handle[b]);
            if(ret == CAEN_DGTZ_Success)
                break;
        }

        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Can't open digitizer on board %1: %2")
                                   .arg(b)
                                   .arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            qDebug() << lastErrorMessage;
            return false;
        }

        qDebug() << "Digitizer opened successfully on board" << b;
        emit progressUpdated(QString("Digitizer opened on board %1").arg(b));
    }

    isOpened_flag = true;
    emit digitizerConnected();
    return true;
}

bool DigitizerOperation::configureDigitizer()
{
    if (!isOpened_flag) {
        lastErrorMessage = "Digitizer not opened. Call openDigitizer() first.";
        emit errorOccurred(lastErrorMessage);
        return false;
    }

    emit progressUpdated("Configuring digitizer...");

    for(int b = 0; b < MAXNB; b++) {
        // Получаем информацию о плате
        ret = CAEN_DGTZ_GetInfo(handle[b], &BoardInfo);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to get board info: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        qDebug() << "Connected to CAEN Digitizer Model" << BoardInfo.ModelName;
        qDebug() << "ROC FPGA Release:" << BoardInfo.ROC_FirmwareRel;
        qDebug() << "AMC FPGA Release:" << BoardInfo.AMC_FirmwareRel;

        emit progressUpdated(QString("Connected to %1").arg(BoardInfo.ModelName));

        // Проверяем версию прошивки (DPP прошивки не поддерживаются)
        int MajorNumber;
        sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
        if (MajorNumber >= 128) {
            lastErrorMessage = "This digitizer has a DPP firmware which is not supported!";
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        // Конфигурация оцифровщика
        ret = CAEN_DGTZ_Reset(handle[b]);
        if(ret != CAEN_DGTZ_Success) {
            qDebug() << "Warning: Reset failed:" << errorCodeToString(ret);
        }

        ret = CAEN_DGTZ_SetRecordLength(handle[b], recLength);  // Длина каждого окна набора в сэмплах
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set record length: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        ret = CAEN_DGTZ_SetChannelEnableMask(handle[b], chMask);  // Включаем канал 0
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set channel enable mask: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle[b], 0, trigThreshold);  // Устанавливаем порог триггера
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set trigger threshold: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        ret = CAEN_DGTZ_SetChannelSelfTrigger(handle[b], CAEN_DGTZ_TRGMODE_ACQ_ONLY, chMask);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set self trigger: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        ret = CAEN_DGTZ_SetSWTriggerMode(handle[b], CAEN_DGTZ_TRGMODE_ACQ_ONLY);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set SW trigger mode: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle[b], 3);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set max events: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        ret = CAEN_DGTZ_SetAcquisitionMode(handle[b], CAEN_DGTZ_SW_CONTROLLED);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to set acquisition mode: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }

        qDebug() << "Digitizer configured successfully for board" << b;
    }

    // Выделяем буфер для чтения данных
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle[0], &buffer, &size);
    if(ret != CAEN_DGTZ_Success) {
        lastErrorMessage = QString("Failed to allocate readout buffer: %1").arg(errorCodeToString(ret));
        emit errorOccurred(lastErrorMessage);
        return false;
    }

    isConfigured_flag = true;
    emit progressUpdated("Digitizer configured successfully");
    return true;
}

bool DigitizerOperation::startAcquisition()
{
    if (!isConfigured_flag) {
        lastErrorMessage = "Digitizer not configured. Call configureDigitizer() first.";
        emit errorOccurred(lastErrorMessage);
        return false;
    }

    if (isAcquiring) {
        emit errorOccurred("Acquisition already in progress");
        return false;
    }

    emit progressUpdated("Starting acquisition...");

    // Запускаем acquisition для всех плат
    for(int b = 0; b < MAXNB; b++) {
        ret = CAEN_DGTZ_SWStartAcquisition(handle[b]);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to start acquisition on board %1: %2")
                                   .arg(b)
                                   .arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }
    }

    isAcquiring = true;

    // Запускаем поток для сбора данных
    acquisitionThread = QThread::create([this]() { acquisitionLoop(); });
    connect(acquisitionThread, &QThread::finished, acquisitionThread, &QThread::deleteLater);
    acquisitionThread->start();

    emit acquisitionStarted();
    emit progressUpdated("Acquisition started");
    return true;
}

void DigitizerOperation::acquisitionLoop()
{
    memset(count, 0, sizeof(count));

    while (isAcquiring) {
        for(int b = 0; b < MAXNB; b++) {
            // Отправляем программный триггер
            ret = CAEN_DGTZ_SendSWtrigger(handle[b]);
            if(ret != CAEN_DGTZ_Success && ret != CAEN_DGTZ_Timeout) {
                emit errorOccurred(QString("Failed to send SW trigger: %1").arg(errorCodeToString(ret)));
                continue;
            }

            // Читаем данные из буфера
            ret = CAEN_DGTZ_ReadData(handle[b], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &bsize);
            if(ret != CAEN_DGTZ_Success && ret != CAEN_DGTZ_Timeout) {
                emit errorOccurred(QString("Failed to read data: %1").arg(errorCodeToString(ret)));
                continue;
            }

            // Получаем количество событий в буфере
            ret = CAEN_DGTZ_GetNumEvents(handle[b], buffer, bsize, &numEvents);
            if(ret != CAEN_DGTZ_Success) {
                emit errorOccurred(QString("Failed to get number of events: %1").arg(errorCodeToString(ret)));
                continue;
            }

            if(numEvents > 0) {
                count[b] += numEvents;

                // Обрабатываем каждое событие
                for(uint32_t i = 0; i < numEvents; i++) {
                    ret = CAEN_DGTZ_GetEventInfo(handle[b], buffer, bsize, i, &eventInfo, &evtptr);
                    if(ret == CAEN_DGTZ_Success) {
                        //void* tmpEvt = NULL;
                        ret = CAEN_DGTZ_DecodeEvent(handle[b], evtptr, reinterpret_cast<void**>(&Evt));//&Evt);
                        if(ret == CAEN_DGTZ_Success) {
                            // TODO: Здесь можно добавить обработку данных события
                            // Например, сохранить waveform или вычислить параметры

                            // Освобождаем память события
                            CAEN_DGTZ_FreeEvent(handle[b], reinterpret_cast<void**>(&Evt));//&Evt);
                        }
                    }
                }

                emit dataAcquired(numEvents);
                emit progressUpdated(QString("Acquired %1 events from board %2 (Total: %3)")
                                         .arg(numEvents).arg(b).arg(count[b]));
            }
        }

        // Небольшая задержка, чтобы не перегружать CPU
        QThread::msleep(100);
    }
}

bool DigitizerOperation::stopAcquisition()
{
    if (!isAcquiring) {
        return true;
    }

    emit progressUpdated("Stopping acquisition...");

    isAcquiring = false;

    if (acquisitionThread && acquisitionThread->isRunning()) {
        acquisitionThread->quit();
        acquisitionThread->wait(5000); // Ждем завершения потока до 5 секунд
    }

    for(int b = 0; b < MAXNB; b++) {
        if (handle[b]) {
            ret = CAEN_DGTZ_SWStopAcquisition(handle[b]);
            if(ret != CAEN_DGTZ_Success) {
                qDebug() << "Warning: Failed to stop acquisition on board" << b << ":" << errorCodeToString(ret);
            }
        }
    }

    // Выводим статистику
    for(int b = 0; b < MAXNB; b++) {
        if(count[b] > 0) {
            qDebug() << "Board" << b << ": Retrieved" << count[b] << "Events";
            emit progressUpdated(QString("Board %1: Retrieved %2 events").arg(b).arg(count[b]));
        }
    }

    emit acquisitionStopped();
    emit progressUpdated("Acquisition stopped");
    return true;
}

bool DigitizerOperation::closeDigitizer()
{
    if (!isOpened_flag) {
        return true;
    }

    if (isAcquiring) {
        stopAcquisition();
    }

    emit progressUpdated("Closing digitizer...");

    for(int b = 0; b < MAXNB; b++) {
        if (handle[b]) {
            ret = CAEN_DGTZ_CloseDigitizer(handle[b]);
            if(ret != CAEN_DGTZ_Success) {
                qDebug() << "Warning: Failed to close digitizer on board" << b << ":" << errorCodeToString(ret);
            }
            handle[b] = 0;
        }
    }

    if (buffer) {
        CAEN_DGTZ_FreeReadoutBuffer(&buffer);
        buffer = nullptr;
    }

    isOpened_flag = false;
    isConfigured_flag = false;
    emit digitizerDisconnected();
    emit progressUpdated("Digitizer closed");
    return true;
}

bool DigitizerOperation::sendSoftwareTrigger()
{
    if (!isAcquiring) {
        emit errorOccurred("Acquisition not started. Call startAcquisition() first.");
        return false;
    }

    for(int b = 0; b < MAXNB; b++) {
        ret = CAEN_DGTZ_SendSWtrigger(handle[b]);
        if(ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QString("Failed to send SW trigger: %1").arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            return false;
        }
    }

    return true;
}

bool DigitizerOperation::readData()
{
    if (!isAcquiring) {
        emit errorOccurred("Acquisition not started. Call startAcquisition() first.");
        return false;
    }

    // Данные читаются автоматически в acquisitionLoop
    // Этот метод можно использовать для принудительного чтения

    return true;
}

bool DigitizerOperation::isOpened() const
{
    return isOpened_flag;
}

bool DigitizerOperation::isConfigured() const
{
    return isConfigured_flag;
}

CAEN_DGTZ_BoardInfo_t DigitizerOperation::getBoardInfo()
{
    return BoardInfo;
}

int DigitizerOperation::getTotalEventsCount() const
{
    int total = 0;
    for(int b = 0; b < MAXNB; b++) {
        total += count[b];
    }
    return total;
}

bool DigitizerOperation::getAcquiringStatus(){
    return this->isAcquiring;
}

bool DigitizerOperation::setRecLength(uint32_t newRecLength){
    this->recLength = newRecLength;
    return true;
}

bool DigitizerOperation::setChMask(uint32_t newChMask){
    this->chMask = newChMask;
    return true;
}

bool DigitizerOperation::setTrigThreshold(uint32_t newTrigThreshold){
    this->trigThreshold = newTrigThreshold;
    return true;
}

uint32_t DigitizerOperation::getRecLength() const{
    return this->recLength;
}

uint32_t DigitizerOperation::getChMask() const{
    return this->chMask;
}

uint32_t DigitizerOperation::getTrigThreshold() const{
    return this->trigThreshold;
}
