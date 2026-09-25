#include "waveformserver.h"

#include <QDataStream>

WaveformServer::WaveformServer(QObject *parent)
    : QObject(parent)
{
    connect(&server, &QTcpServer::newConnection,
            this, &WaveformServer::acceptConnection);
}

bool WaveformServer::listen(quint16 port, const QHostAddress &address)
{
    return server.listen(address, port);
}

bool WaveformServer::takeNextEvent(DigitizerProtocol::Event &event)
{
    if (events.isEmpty())
        return false;

    event = events.dequeue();
    return true;
}

qsizetype WaveformServer::pendingEventCount() const
{
    return events.size();
}

void WaveformServer::acceptConnection()
{
    QTcpSocket *incoming = server.nextPendingConnection();

    if (client) {
        disconnect(client, nullptr, this, nullptr);
        client->disconnectFromHost();
        client->deleteLater();
    }

    client = incoming;
    inputBuffer.clear();

    connect(client, &QTcpSocket::readyRead,
            this, &WaveformServer::readClientData);

    connect(client, &QTcpSocket::disconnected,
            this, &WaveformServer::closeClient);

    emit clientConnected();
}

void WaveformServer::readClientData()
{
    if (!client)
        return;

    inputBuffer.append(client->readAll());
    parseInputBuffer();
}

void WaveformServer::closeClient()
{
    if (!client)
        return;

    client->deleteLater();
    client = nullptr;
    inputBuffer.clear();

    emit clientDisconnected();
}

void WaveformServer::parseInputBuffer()
{
    constexpr qsizetype HeaderSize =
        sizeof(quint32) +
        sizeof(quint16) +
        sizeof(quint16) +
        sizeof(quint32);

    while (inputBuffer.size() >= HeaderSize) {
        const QByteArray headerBytes = inputBuffer.left(HeaderSize);

        QDataStream headerStream(headerBytes);
        headerStream.setVersion(QDataStream::Qt_5_15);

        quint32 magic = 0;
        quint16 version = 0;
        quint16 type = 0;
        quint32 payloadSize = 0;

        headerStream >> magic >> version >> type >> payloadSize;

        const bool validHeader =
            magic == DigitizerProtocol::Magic &&
            version == DigitizerProtocol::Version &&
            type == static_cast<quint16>(
                        DigitizerProtocol::MessageType::WaveformEvent) &&
            payloadSize <= DigitizerProtocol::MaxPayloadBytes;

        if (!validHeader) {
            inputBuffer.remove(0, 1);
            emit protocolError(
                QStringLiteral("Invalid TCP packet header; resynchronizing"));
            continue;
        }

        const qsizetype fullPacketSize =
            HeaderSize + static_cast<qsizetype>(payloadSize);

        if (inputBuffer.size() < fullPacketSize)
            return;

        const QByteArray payload =
            inputBuffer.mid(HeaderSize, static_cast<qsizetype>(payloadSize));

        inputBuffer.remove(0, fullPacketSize);

        DigitizerProtocol::Event event;

        QDataStream payloadStream(payload);
        payloadStream.setVersion(QDataStream::Qt_5_15);
        payloadStream >> event;

        const bool validPayload =
            payloadStream.status() == QDataStream::Ok &&
            !event.samples.isEmpty() &&
            static_cast<quint32>(event.samples.size()) <=
                DigitizerProtocol::MaxSamples;

        if (!validPayload) {
            emit protocolError(QStringLiteral("Invalid waveform payload"));
            continue;
        }

        if (events.size() >= MaxQueuedEvents)
            events.dequeue();

        events.enqueue(std::move(event));
    }
}