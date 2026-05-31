#ifndef DIGITIZEROPERATION_H
#define DIGITIZEROPERATION_H

#include "CAENDigitizer.h"
#include <iostream>
#include <QDebug>
#include <QThread>
#include <QVector>

#define MAXNB 1  // Количество подключенных плат

class DigitizerOperation : public QObject
{
    Q_OBJECT

private:
    CAEN_DGTZ_ErrorCode ret;
    CAEN_DGTZ_BoardInfo_t BoardInfo;
    CAEN_DGTZ_EventInfo_t eventInfo;
    CAEN_DGTZ_UINT16_EVENT_t *Evt = NULL;

    uint32_t recLength = 4096;
    uint32_t chMask = 0x1;
    uint32_t trigThreshold = 32768;


    char *buffer = NULL;
    int handle[MAXNB];
    int count[MAXNB];
    char *evtptr = NULL;
    uint32_t size, bsize;
    uint32_t numEvents;
    bool isAcquiring;
    QThread* acquisitionThread;


public:
    explicit DigitizerOperation(QObject *parent = nullptr);
    ~DigitizerOperation();

    // Основные методы для работы с оцифровщиком
    bool openDigitizer();
    bool configureDigitizer();
    bool startAcquisition();
    bool stopAcquisition();
    bool closeDigitizer();
    bool sendSoftwareTrigger();
    bool readData();
    bool isOpened() const;
    bool isConfigured() const;
    bool setRecLength(uint32_t newRecLength);
    bool setChMask(uint32_t newChMask);
    bool setTrigThreshold(uint32_t newTrigThreshold);

    // Геттеры
    CAEN_DGTZ_ErrorCode getLastError();
    QString getLastErrorMessage();
    CAEN_DGTZ_BoardInfo_t getBoardInfo();
    int getTotalEventsCount() const;
    bool getAcquiringStatus();
    uint32_t getRecLength() const;
    uint32_t getChMask() const;
    uint32_t getTrigThreshold() const;

signals:
    void acquisitionStarted();
    void acquisitionStopped();
    void dataAcquired(int eventsCount);
    void digitizerConnected();
    void digitizerDisconnected();
    void errorOccurred(QString errorMessage);
    void progressUpdated(QString message);

private slots:
    void acquisitionLoop();

private:
    bool isOpened_flag;
    bool isConfigured_flag;
    QString lastErrorMessage;

    QString errorCodeToString(CAEN_DGTZ_ErrorCode code);
};

#endif // DIGITIZEROPERATION_H
