#ifndef DIGITIZEROPERATION_H
#define DIGITIZEROPERATION_H

#include "CAENDigitizer.h"

#include <QObject>
#include <QThread>
#include <QString>
#include <QTcpSocket>
#include <QHostAddress>
#include <atomic>
#include <cstdint>

#define MAXNB 1
#define ZERO_TRESHOLD_STATE 0x7FFF

class DigitizerOperation : public QObject
{
    Q_OBJECT

public:
    explicit DigitizerOperation(QObject *parent = nullptr);
    ~DigitizerOperation() override;

    bool openDigitizer();
    bool configureDigitizer();
    bool startAcquisition();
    bool stopAcquisition();
    bool closeDigitizer();
    bool sendSoftwareTrigger();
    bool readData();

    bool setRecLength(uint32_t newRecLength);
    bool setChMask(uint32_t newChMask);
    bool setTrigThreshold(uint32_t newTrigThreshold);
    void setVisualizationEndpoint(const QString &host, quint16 port);
    bool setPostTrigSize(uint16_t newPostTrigSize);
    bool setGatePosition(uint16_t newGatePosition);
    bool setGateWidth(uint16_t newGateWidth);

    bool openVisualizationConnection();
    void closeVisualizationConnection();

    bool isOpened() const;
    bool isConfigured() const;
    bool getAcquiringStatus() const;
    CAEN_DGTZ_ErrorCode getLastError() const;
    QString getLastErrorMessage() const;
    CAEN_DGTZ_BoardInfo_t getBoardInfo() const;
    int getTotalEventsCount() const;
    uint32_t getRecLength() const;
    uint32_t getChMask() const;
    uint32_t getTrigThreshold() const;
    uint16_t getPostTrigSize() const;
    uint16_t getGatePosition() const;
    uint16_t getGateWidth() const;

signals:
    void acquisitionStarted();
    void acquisitionStopped();
    void dataAcquired(int eventsCount);
    void digitizerConnected();
    void digitizerDisconnected();
    void errorOccurred(const QString &errorMessage);
    void progressUpdated(const QString &message);
    void visualizationConnected();
    void visualizationDisconnected();
    void waveformSent(quint64 eventId);

private:
    void acquisitionLoop();
    bool sendWaveform(quint16 board, quint16 channel,
                      quint64 timestamp, const CAEN_DGTZ_UINT16_EVENT_t &event);
    QString errorCodeToString(CAEN_DGTZ_ErrorCode code) const;

    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    CAEN_DGTZ_BoardInfo_t boardInfo{};
    CAEN_DGTZ_EventInfo_t eventInfo{};
    CAEN_DGTZ_UINT16_EVENT_t *decodedEvent = nullptr;

    uint32_t recLength = 8192;
    uint32_t chMask = 0x01; //one bit per channel: 0x01 0x02 0x04 0x08
    uint32_t trigThreshold = ZERO_TRESHOLD_STATE; //zero state
    uint32_t postTrigSize = 50; //percentage of the post trigger size

    char *buffer = nullptr;
    int handle[MAXNB]{};
    int count[MAXNB]{};
    uint32_t readoutBufferSize = 0;

    std::atomic_bool isAcquiring{false};
    bool isOpenedFlag = false;
    bool isConfiguredFlag = false;
    QThread *acquisitionThread = nullptr;
    QString lastErrorMessage;

    quint64 nextEventId = 0;
    uint16_t gateWidth = 50;
    uint16_t gatePosition = 50;

public:
    QString visualizationHost = QStringLiteral("127.0.0.1");
    quint16 visualizationPort = 45454;
    QTcpSocket *visualizationSocket = nullptr;



};

#endif // DIGITIZEROPERATION_H
