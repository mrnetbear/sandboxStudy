#include "digitizeroperation.h"
#include "digitizerprotocol.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <cstring>

DigitizerOperation::DigitizerOperation(QObject *parent)
    : QObject(parent)
{
}

DigitizerOperation::~DigitizerOperation()
{
    stopAcquisition();
    closeDigitizer();
}

QString DigitizerOperation::errorCodeToString(CAEN_DGTZ_ErrorCode code) const
{
    switch (code) {
    case CAEN_DGTZ_Success: return QStringLiteral("Success");
    case CAEN_DGTZ_Timeout: return QStringLiteral("Timeout");
    case CAEN_DGTZ_EventNotFound: return QStringLiteral("Event not found");
    case CAEN_DGTZ_InvalidHandle: return QStringLiteral("Invalid handle");
    case CAEN_DGTZ_InvalidParam: return QStringLiteral("Invalid parameter");
    case CAEN_DGTZ_CalibrationError: return QStringLiteral("Calibration/communication error");
    case CAEN_DGTZ_UnsupportedTrace: return QStringLiteral("Unsupported trace");
    default: return QStringLiteral("CAEN error %1").arg(static_cast<int>(code));
    }
}

CAEN_DGTZ_ErrorCode DigitizerOperation::getLastError() const { return ret; }
QString DigitizerOperation::getLastErrorMessage() const { return lastErrorMessage; }
CAEN_DGTZ_BoardInfo_t DigitizerOperation::getBoardInfo() const { return boardInfo; }
bool DigitizerOperation::isOpened() const { return isOpenedFlag; }
bool DigitizerOperation::isConfigured() const { return isConfiguredFlag; }
bool DigitizerOperation::getAcquiringStatus() const { return isAcquiring.load(); }
uint32_t DigitizerOperation::getRecLength() const { return recLength; }
uint32_t DigitizerOperation::getChMask() const { return chMask; }
uint32_t DigitizerOperation::getTrigThreshold() const { return trigThreshold; }

int DigitizerOperation::getTotalEventsCount() const
{
    int total = 0;
    for (int boardCount : count)
        total += boardCount;
    return total;
}

bool DigitizerOperation::setRecLength(uint32_t value)
{
    if (value == 0) {
        emit errorOccurred(QStringLiteral("Record length must be positive"));
        return false;
    }
    if (isConfiguredFlag || isAcquiring.load()) {
        emit errorOccurred(QStringLiteral("Stop acquisition and reconnect before changing record length"));
        return false;
    }
    recLength = value;
    return true;
}

bool DigitizerOperation::setChMask(uint32_t value)
{
    if (value == 0) {
        emit errorOccurred(QStringLiteral("At least one channel must be enabled"));
        return false;
    }
    if (isConfiguredFlag || isAcquiring.load()) {
        emit errorOccurred(QStringLiteral("Stop acquisition and reconnect before changing channel mask"));
        return false;
    }
    chMask = value;
    return true;
}

bool DigitizerOperation::setTrigThreshold(uint32_t value)
{
    if (value > 65535U) {
        emit errorOccurred(QStringLiteral("Trigger threshold must be in [0, 65535]"));
        return false;
    }
    if (isConfiguredFlag || isAcquiring.load()) {
        emit errorOccurred(QStringLiteral("Stop acquisition and reconnect before changing trigger threshold"));
        return false;
    }
    trigThreshold = value;
    return true;
}

void DigitizerOperation::setVisualizationEndpoint(const QString &host, quint16 port)
{
    visualizationHost = host;
    visualizationPort = port;
}

bool DigitizerOperation::openDigitizer()
{
    if (isOpenedFlag)
        return true;

    emit progressUpdated(QStringLiteral("Opening digitizer..."));
    for (int board = 0; board < MAXNB; ++board) {
        ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, board, 0, 0, &handle[board]);
        if (ret != CAEN_DGTZ_Success) {
            lastErrorMessage = QStringLiteral("Cannot open CAEN board %1: %2")
                                   .arg(board).arg(errorCodeToString(ret));
            emit errorOccurred(lastErrorMessage);
            closeDigitizer();
            return false;
        }
    }

    isOpenedFlag = true;
    emit digitizerConnected();
    return true;
}

bool DigitizerOperation::configureDigitizer()
{
    if (!isOpenedFlag) {
        emit errorOccurred(QStringLiteral("Digitizer is not open"));
        return false;
    }

    for (int board = 0; board < MAXNB; ++board) {
        ret = CAEN_DGTZ_GetInfo(handle[board], &boardInfo);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("CAEN_DGTZ_GetInfo failed: %1").arg(errorCodeToString(ret)));
            return false;
        }

        ret = CAEN_DGTZ_Reset(handle[board]);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("CAEN_DGTZ_Reset failed: %1").arg(errorCodeToString(ret)));
            return false;
        }

        ret = CAEN_DGTZ_SetRecordLength(handle[board], recLength);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("Cannot set record length: %1").arg(errorCodeToString(ret)));
            return false;
        }

        ret = CAEN_DGTZ_SetChannelEnableMask(handle[board], chMask);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("Cannot set channel mask: %1").arg(errorCodeToString(ret)));
            return false;
        }

        for (unsigned channel = 0; channel < 32; ++channel) {
            if ((chMask & (1U << channel)) == 0)
                continue;
            ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle[board], channel, trigThreshold);
            if (ret != CAEN_DGTZ_Success) {
                emit errorOccurred(QStringLiteral("Cannot set threshold for channel %1: %2")
                                       .arg(channel).arg(errorCodeToString(ret)));
                return false;
            }
        }

        ret = CAEN_DGTZ_SetChannelSelfTrigger(handle[board], CAEN_DGTZ_TRGMODE_ACQ_ONLY, chMask);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("Cannot enable self trigger: %1").arg(errorCodeToString(ret)));
            return false;
        }
        /********************************************
        ret = CAEN_DGTZ_SetSWTriggerMode(handle[board], CAEN_DGTZ_TRGMODE_ACQ_ONLY);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(
                QStringLiteral("Cannot configure software trigger: %1")
                    .arg(errorCodeToString(ret)));
            return false;
        }
        ********************************************/

        ret = CAEN_DGTZ_SetAcquisitionMode(handle[board], CAEN_DGTZ_SW_CONTROLLED);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("Cannot set acquisition mode: %1").arg(errorCodeToString(ret)));
            return false;
        }
    }

    ret = CAEN_DGTZ_MallocReadoutBuffer(handle[0], &buffer, &readoutBufferSize);
    if (ret != CAEN_DGTZ_Success) {
        emit errorOccurred(QStringLiteral("Cannot allocate readout buffer: %1").arg(errorCodeToString(ret)));
        return false;
    }

    isConfiguredFlag = true;
    emit progressUpdated(QStringLiteral("Digitizer configured"));
    return true;
}

bool DigitizerOperation::openVisualizationConnection()
{
    if (visualizationSocket && visualizationSocket->state() == QAbstractSocket::ConnectedState)
        return true;

    closeVisualizationConnection();
    visualizationSocket = new QTcpSocket;
    visualizationSocket->connectToHost(visualizationHost, visualizationPort);
    if (!visualizationSocket->waitForConnected(1000)) {
        delete visualizationSocket;
        visualizationSocket = nullptr;
        return false;
    }

    emit visualizationConnected();
    emit progressUpdated(QStringLiteral("Connected to visualizer at %1:%2")
                             .arg(visualizationHost).arg(visualizationPort));
    return true;
}

void DigitizerOperation::closeVisualizationConnection()
{
    if (!visualizationSocket)
        return;
    visualizationSocket->disconnectFromHost();
    visualizationSocket->waitForDisconnected(100);
    delete visualizationSocket;
    visualizationSocket = nullptr;
    emit visualizationDisconnected();
}

bool DigitizerOperation::startAcquisition()
{
    if (!isConfiguredFlag || isAcquiring.load())
        return false;

    for (int board = 0; board < MAXNB; ++board) {
        ret = CAEN_DGTZ_SWStartAcquisition(handle[board]);
        if (ret != CAEN_DGTZ_Success) {
            emit errorOccurred(QStringLiteral("Cannot start acquisition: %1").arg(errorCodeToString(ret)));
            return false;
        }
    }

    isAcquiring.store(true);
    acquisitionThread = QThread::create([this] { acquisitionLoop(); });
    connect(acquisitionThread, &QThread::finished, acquisitionThread, &QObject::deleteLater);
    acquisitionThread->start();

    emit acquisitionStarted();
    return true;
}

bool DigitizerOperation::sendWaveform(quint16 board, quint16 channel,
                                      quint64 timestamp,
                                      const CAEN_DGTZ_UINT16_EVENT_t &event)
{
    if (!event.DataChannel[channel] || event.ChSize[channel] == 0)
        return true;

    if (!openVisualizationConnection())
        return false; // acquisition must continue even if GUI is absent

    DigitizerProtocol::Event packetEvent;
    packetEvent.eventId = ++nextEventId;
    packetEvent.timestamp = timestamp;
    packetEvent.board = board;
    packetEvent.channel = channel;
    packetEvent.adcBits = 16;
    packetEvent.samples.resize(static_cast<qsizetype>(event.ChSize[channel]));

    for (uint32_t i = 0; i < event.ChSize[channel]; ++i)
        packetEvent.samples[static_cast<qsizetype>(i)] = event.DataChannel[channel][i];

    const QByteArray packet = DigitizerProtocol::makePacket(packetEvent);
    if (visualizationSocket->write(packet) != packet.size() ||
        !visualizationSocket->waitForBytesWritten(100)) {
        closeVisualizationConnection();
        return false;
    }

    emit waveformSent(packetEvent.eventId);
    return true;
}

void DigitizerOperation::acquisitionLoop()
{
    std::memset(count, 0, sizeof(count));

    while (isAcquiring.load()) {
        for (int board = 0; board < MAXNB; ++board) {

            /**********************************************
            ret = CAEN_DGTZ_SendSWtrigger(handle[board]);
            if (ret != CAEN_DGTZ_Success && ret != CAEN_DGTZ_Timeout) {
                emit errorOccurred(
                    QStringLiteral("SendSWtrigger failed for board %1: %2")
                        .arg(board)
                        .arg(errorCodeToString(ret)));
                continue;
            }
            **********************************************/

            uint32_t blockSize = 0;
            ret = CAEN_DGTZ_ReadData(handle[board], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                                     buffer, &blockSize);

            if (ret != CAEN_DGTZ_Success) {
                if (ret != CAEN_DGTZ_Timeout)
                    emit errorOccurred(QStringLiteral("ReadData failed: %1").arg(errorCodeToString(ret)));
                continue;
            }

            uint32_t eventCount = 0;
            ret = CAEN_DGTZ_GetNumEvents(handle[board], buffer, blockSize, &eventCount);
            if (ret != CAEN_DGTZ_Success) {
                emit errorOccurred(QStringLiteral("GetNumEvents failed: %1").arg(errorCodeToString(ret)));
                continue;
            }

            for (uint32_t eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
                char *eventPtr = nullptr;
                ret = CAEN_DGTZ_GetEventInfo(handle[board], buffer, blockSize, eventIndex,
                                             &eventInfo, &eventPtr);
                if (ret != CAEN_DGTZ_Success)
                    continue;

                decodedEvent = nullptr;
                ret = CAEN_DGTZ_DecodeEvent(handle[board], eventPtr,
                                            reinterpret_cast<void **>(&decodedEvent));
                if (ret != CAEN_DGTZ_Success || !decodedEvent)
                    continue;

                for (quint16 channel = 0; channel < 32; ++channel) {
                    if ((chMask & (1U << channel)) == 0)
                        continue;
                    sendWaveform(static_cast<quint16>(board), channel,
                                 static_cast<quint64>(eventInfo.TriggerTimeTag), *decodedEvent);
                }

                CAEN_DGTZ_FreeEvent(handle[board], reinterpret_cast<void **>(&decodedEvent));
                decodedEvent = nullptr;
                ++count[board];
            }

            if (eventCount > 0)
                emit dataAcquired(static_cast<int>(eventCount));
        }
        //QThread::msleep(1); //modify for data acuisition speed
    }

    closeVisualizationConnection();
}

bool DigitizerOperation::stopAcquisition()
{
    if (!isAcquiring.exchange(false))
        return true;

    if (acquisitionThread && acquisitionThread->isRunning())
        acquisitionThread->wait(5000);
    acquisitionThread = nullptr;

    for (int board = 0; board < MAXNB; ++board)
        CAEN_DGTZ_SWStopAcquisition(handle[board]);

    emit acquisitionStopped();
    return true;
}

bool DigitizerOperation::closeDigitizer()
{
    if (!isOpenedFlag)
        return true;

    stopAcquisition();
    if (buffer) {
        CAEN_DGTZ_FreeReadoutBuffer(&buffer);
        buffer = nullptr;
    }
    for (int board = 0; board < MAXNB; ++board) {
        if (handle[board] != 0) {
            CAEN_DGTZ_CloseDigitizer(handle[board]);
            handle[board] = 0;
        }
    }
    isOpenedFlag = false;
    isConfiguredFlag = false;
    emit digitizerDisconnected();
    return true;
}

bool DigitizerOperation::readData()
{
    return isAcquiring.load();
}

bool DigitizerOperation::sendSoftwareTrigger()
{
    if (!isAcquiring.load())
        return false;
    for (int board = 0; board < MAXNB; ++board) {
        ret = CAEN_DGTZ_SendSWtrigger(handle[board]);
        if (ret != CAEN_DGTZ_Success)
            return false;
    }
    return true;
}
