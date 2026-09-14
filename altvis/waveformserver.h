#ifndef WAVEFORMSERVER_H
#define WAVEFORMSERVER_H

#include "digitizerprotocol.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QQueue>

class WaveformServer : public QObject
{
    Q_OBJECT

public:
    explicit WaveformServer(QObject *parent = nullptr);
    bool listen(quint16 port = 45454,
                const QHostAddress &address = QHostAddress::LocalHost);
    bool takeNextEvent(DigitizerProtocol::Event &event);
    qsizetype pendingEventCount() const;

signals:
    void clientConnected();
    void clientDisconnected();
    void protocolError(const QString &message);

private slots:
    void acceptConnection();
    void readClientData();
    void closeClient();

private:
    void parseInputBuffer();

    QTcpServer server;
    QTcpSocket *client = nullptr;
    QByteArray inputBuffer;
    QQueue<DigitizerProtocol::Event> events;
    static constexpr qsizetype MaxQueuedEvents = 10000;
};

#endif // WAVEFORMSERVER_H