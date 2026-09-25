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
#include <array>
#include <cstdint>

#define MAXNB 1

class DigitizerOperation : public QObject
{
    Q_OBJECT

public:

    enum class TriggerMode {
        Software,
        Self,
        Both
    };

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

    bool setChannelEnabled(unsigned channel, bool enabled);
    bool setChannelSelfTriggerEnabled(unsigned channel, bool enabled);
    bool setChannelThreshold(unsigned channel, uint32_t threshold);
    bool setTriggerMode(TriggerMode mode);

    uint32_t channelMask() const { return chMask; }
    uint32_t selfTriggerMask() const { return selfTriggerMask_; }
    uint32_t channelThreshold(unsigned channel) const;
    TriggerMode triggerMode() const { return triggerMode_; }
    unsigned channelCount() const;

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
    bool openVisualizationConnection();
    void closeVisualizationConnection();
    bool sendWaveform(quint16 board, quint16 channel,
                      quint64 timestamp, const CAEN_DGTZ_UINT16_EVENT_t &event);
    QString errorCodeToString(CAEN_DGTZ_ErrorCode code) const;


    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    CAEN_DGTZ_BoardInfo_t boardInfo{};
    CAEN_DGTZ_EventInfo_t eventInfo{};
    CAEN_DGTZ_UINT16_EVENT_t *decodedEvent = nullptr;

    static constexpr unsigned MaxChannels = 32;

    uint32_t recLength = 5000;
    uint32_t trigThreshold = 32768;
    uint32_t chMask = 0x1;
    uint32_t selfTriggerMask_ = 0x1;
    std::array<uint32_t, MaxChannels> channelThresholds_{};
    TriggerMode triggerMode_ = TriggerMode::Software;

    bool canChangeHardwareSettings() const;
    bool configureTriggers(int board);

    char *buffer = nullptr;
    int handle[MAXNB]{};
    int count[MAXNB]{};
    uint32_t readoutBufferSize = 0;

    std::atomic_bool isAcquiring{false};
    bool isOpenedFlag = false;
    bool isConfiguredFlag = false;
    QThread *acquisitionThread = nullptr;
    QString lastErrorMessage;

    QString visualizationHost = QStringLiteral("127.0.0.1");
    quint16 visualizationPort = 45454;
    QTcpSocket *visualizationSocket = nullptr;
    quint64 nextEventId = 0;
};

#endif // DIGITIZEROPERATION_H
