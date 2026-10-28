#ifndef SETTINGSSERVER_H
#define SETTINGSSERVER_H

#include "digitizerprotocol.h"

#include <QByteArray>
#include <QDataStream>
#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <functional>
#include <utility>

class SettingsServer : public QObject
{
public:
    using Handler =
        std::function<void(const DigitizerProtocol::Params &)>;

    explicit SettingsServer(Handler handler,
                            QObject *parent = nullptr)
        : QObject(parent),
          server(this),
          handler(std::move(handler))
    {
        connect(
            &server,
            &QTcpServer::newConnection,
            this,
            [this] {
                while (server.hasPendingConnections()) {
                    QTcpSocket *incoming =
                        server.nextPendingConnection();

                    if (client) {
                        QObject::disconnect(
                            client, nullptr, this, nullptr);

                        client->abort();
                        client->deleteLater();
                    }

                    client = incoming;
                    input.clear();

                    connect(
                        incoming,
                        &QTcpSocket::readyRead,
                        this,
                        [this, incoming] {
                            if (client != incoming)
                                return;

                            input.append(incoming->readAll());

                            if (input.size() > 65536) {
                                incoming->abort();
                                return;
                            }

                            parseInput();
                        });

                    connect(
                        incoming,
                        &QTcpSocket::disconnected,
                        this,
                        [this, incoming] {
                            if (client == incoming) {
                                client = nullptr;
                                input.clear();
                            }

                            incoming->deleteLater();
                        });
                }
            });
    }

    bool listen(quint16 port = 46464)
    {
        return server.listen(QHostAddress::LocalHost, port);
    }

    QString errorString() const
    {
        return server.errorString();
    }

private:
    void parseInput()
    {
        constexpr int HeaderSize = 12;

        // Params сериализуется как три 16-битных числа:
        // channel, gateWidth, gatePosition.
        constexpr quint32 PayloadSize = 6;

        while (input.size() >= HeaderSize) {
            QDataStream header(input.left(HeaderSize));
            header.setVersion(QDataStream::Qt_5_15);

            quint32 magic = 0;
            quint16 version = 0;
            quint16 type = 0;
            quint32 payloadSize = 0;

            header >> magic >> version >> type >> payloadSize;

            const bool validHeader =
                header.status() == QDataStream::Ok &&
                magic == DigitizerProtocol::Magic &&
                version == DigitizerProtocol::Version &&
                type == static_cast<quint16>(
                    DigitizerProtocol::MessageType::Parameters) &&
                payloadSize == PayloadSize;

            if (!validHeader) {
                input.clear();

                if (client)
                    client->abort();

                return;
            }

            const int packetSize =
                HeaderSize + static_cast<int>(payloadSize);

            if (input.size() < packetSize)
                return;

            const QByteArray payload =
                input.mid(HeaderSize, payloadSize);

            input.remove(0, packetSize);

            DigitizerProtocol::Params params;

            QDataStream stream(payload);
            stream.setVersion(QDataStream::Qt_5_15);
            stream >> params;

            const bool validParams =
                stream.status() == QDataStream::Ok &&
                stream.atEnd() &&
                params.channel < 4 &&
                params.gateWidth >= 1 &&
                params.gateWidth <= 100 &&
                params.gatePosition <= 100;

            if (!validParams)
                continue;

            if (handler)
                handler(params);
        }
    }

    QTcpServer server;
    QTcpSocket *client = nullptr;
    QByteArray input;
    Handler handler;
};

#endif // SETTINGSSERVER_H