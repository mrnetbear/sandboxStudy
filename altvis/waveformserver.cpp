#include "waveformserver.h"

#include <QDataStream>

WaveformServer::WaveformServer(QObject *parent)
    : QObject(parent)
{
    connect(&server, &QTcpServer::newConnection,
            this, &WaveformServer::acceptConnection);
}

bool WaveformServer::listen(
    quint16 port, const QHostAddress &address)
{
    return server.listen(address, port);
}

void WaveformServer::acceptConnection()
{
    while (server.hasPendingConnections()) {
        QTcpSocket *incoming = server.nextPendingConnection();

        if (client) {
            QObject::disconnect(client, nullptr, this, nullptr);
            client->abort();
            client->deleteLater();
        }

        client = incoming;
        inputBuffer.clear();

        client->setReadBufferSize(
            static_cast<qint64>(
                DigitizerProtocol::MaxPayloadBytes) + 12);

        connect(client, &QTcpSocket::readyRead,
                this, &WaveformServer::readClientData);

        connect(client, &QTcpSocket::disconnected,
                this, &WaveformServer::closeClient);

        emit clientConnected();

        if (client->bytesAvailable() > 0)
            readClientData();
    }
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
    constexpr int HeaderSize = 12;
    constexpr quint32 ParametersPayloadSize = 6;
    constexpr quint32 EventFixedPayloadSize = 26;

    while (inputBuffer.size() >= HeaderSize) {
        QDataStream header(inputBuffer.left(HeaderSize));
        header.setVersion(QDataStream::Qt_5_15);

        quint32 magic = 0;
        quint16 version = 0;
        quint16 type = 0;
        quint32 payloadSize = 0;

        header >> magic >> version >> type >> payloadSize;

        if (header.status() != QDataStream::Ok ||
            magic != DigitizerProtocol::Magic ||
            version != DigitizerProtocol::Version ||
            payloadSize > DigitizerProtocol::MaxPayloadBytes) {

            emit protocolError(
                QStringLiteral("Invalid packet header or protocol version"));

            // Не пытаемся искать Magic внутри повреждённых ADC-данных.
            inputBuffer.clear();
            client->abort();
            return;
        }

        const qint64 packetSize =
            HeaderSize + static_cast<qint64>(payloadSize);

        if (inputBuffer.size() < packetSize)
            return;

        const QByteArray payload =
            inputBuffer.mid(HeaderSize, static_cast<int>(payloadSize));

        inputBuffer.remove(0, static_cast<int>(packetSize));

        const auto messageType =
            static_cast<DigitizerProtocol::MessageType>(type);

        if (messageType ==
            DigitizerProtocol::MessageType::WaveformEvent) {

            // Для текущего Event:
            // 8 + 8 + 2 + 2 + 2 байта полей,
            // затем 4 байта количества элементов QVector<quint16>.
            if (payloadSize < EventFixedPayloadSize) {
                emit protocolError(
                    QStringLiteral("Waveform payload is too short"));
                continue;
            }

            // Проверяем длину массива до его выделения оператором >>.
            QDataStream lengthStream(payload.mid(22, 4));
            lengthStream.setVersion(QDataStream::Qt_5_15);

            quint32 sampleCount = 0;
            lengthStream >> sampleCount;

            if (sampleCount == 0 ||
                sampleCount > DigitizerProtocol::MaxSamples ||
                static_cast<quint64>(payloadSize) !=
                    EventFixedPayloadSize +
                    2ULL * sampleCount) {

                emit protocolError(
                    QStringLiteral("Invalid waveform sample count"));
                continue;
            }

            DigitizerProtocol::Event event;

            QDataStream stream(payload);
            stream.setVersion(QDataStream::Qt_5_15);
            stream >> event;

            if (stream.status() != QDataStream::Ok ||
                !stream.atEnd()) {
                emit protocolError(
                    QStringLiteral("Cannot decode waveform"));
                continue;
            }

            emit waveformReceived(event);
        }
        else if (messageType ==
                 DigitizerProtocol::MessageType::Parameters) {

            // Три quint16: channel, gateWidth, gatePosition.
            if (payloadSize != ParametersPayloadSize) {
                emit protocolError(
                    QStringLiteral("Invalid parameters payload size"));
                continue;
            }

            DigitizerProtocol::Params params;

            QDataStream stream(payload);
            stream.setVersion(QDataStream::Qt_5_15);
            stream >> params;

            if (stream.status() != QDataStream::Ok ||
                !stream.atEnd() ||
                params.gateWidth < 1 ||
                params.gateWidth > 100 ||
                params.gatePosition > 100) {

                emit protocolError(
                    QStringLiteral("Invalid gate parameters"));
                continue;
            }

            emit parametersReceived(params);
        }
        else {
            // Пакет целиком уже извлечён: можно безопасно его пропустить.
            emit protocolError(
                QStringLiteral("Unknown message type: %1").arg(type));
        }
    }
}