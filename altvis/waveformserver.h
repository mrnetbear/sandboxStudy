#ifndef WAVEFORMSERVER_H
#define WAVEFORMSERVER_H

#include "digitizerprotocol.h"

#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class WaveformServer : public QObject
{
    Q_OBJECT

public:
    explicit WaveformServer(QObject *parent = nullptr);

    bool listen(
        quint16 port = 45454,
        const QHostAddress &address = QHostAddress::LocalHost);

signals:
    void clientConnected();
    void clientDisconnected();

    void protocolError(const QString &message);

    void waveformReceived(const DigitizerProtocol::Event &event);
    void parametersReceived(const DigitizerProtocol::Params &params);

private:
    void acceptConnection();
    void readClientData();
    void closeClient();
    void parseInputBuffer();

    QTcpServer server;
    QTcpSocket *client = nullptr;
    QByteArray inputBuffer;
};

#endif // WAVEFORMSERVER_H