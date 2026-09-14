#ifndef DIGITIZERPROTOCOL_H
#define DIGITIZERPROTOCOL_H


#include <QByteArray>
#include <QIODevice>
#include <QDataStream>
#include <QVector>
#include <QtGlobal>

namespace DigitizerProtocol {

constexpr quint32 Magic = 0x4341454E;       // "CAEN"
constexpr quint16 Version = 1;
constexpr quint32 MaxSamples = 1U << 20;    // защита при повреждённом пакете
constexpr quint32 MaxPayloadBytes = 2U * 1024U * 1024U;

enum class MessageType : quint16 {
    WaveformEvent = 1
};

struct Event {
    quint64 eventId = 0;
    quint64 timestamp = 0;
    quint16 board = 0;
    quint16 channel = 0;
    quint16 adcBits = 16;
    QVector<quint16> samples;
};

inline QDataStream &operator<<(QDataStream &out, const Event &event)
{
    out << event.eventId
        << event.timestamp
        << event.board
        << event.channel
        << event.adcBits
        << event.samples;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Event &event)
{
    in >> event.eventId
       >> event.timestamp
       >> event.board
       >> event.channel
       >> event.adcBits
       >> event.samples;
    return in;
}

inline QByteArray makePacket(const Event &event)
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(QDataStream::Qt_5_15);
    payloadStream << event;

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << Magic
           << Version
           << static_cast<quint16>(MessageType::WaveformEvent)
           << static_cast<quint32>(payload.size());
    packet.append(payload);
    return packet;
}

} // namespace DigitizerProtocol

#endif // DIGITIZERPROTOCOL_H