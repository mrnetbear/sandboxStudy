#include "rootcontrol.h"
#include "waveformserver.h"

#include <QCoreApplication>
#include <QTimer>

#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    RootWidget rootWidget;
    WaveformServer server;

    quint16 selectedChannel = 0;
    bool displaysDirty = true;

    QObject::connect(
        &server,
        &WaveformServer::parametersReceived,
        &application,
        [&](const DigitizerProtocol::Params &params) {
            const bool channelChanged =
                selectedChannel != params.channel;

            selectedChannel = params.channel;

            if (channelChanged)
                rootWidget.clearForChannelChange();

            rootWidget.setGatePercent(
                params.gatePosition,
                params.gateWidth);

            displaysDirty = true;

            std::cout
                << "Parameters: channel=" << params.channel
                << ", position=" << params.gatePosition << "%"
                << ", width=" << params.gateWidth << "%"
                << std::endl;
        });

    QObject::connect(
        &server,
        &WaveformServer::waveformReceived,
        &application,
        [&](const DigitizerProtocol::Event &event) {
            if (event.channel != selectedChannel)
                return;

            std::vector<double> samples;
            samples.reserve(
                static_cast<size_t>(event.samples.size()));

            for (quint16 sample : event.samples)
                samples.push_back(static_cast<double>(sample));

            rootWidget.processWaveform(
                samples,
                event.timestamp,
                event.board,
                event.channel);

            displaysDirty = true;
        });

    QObject::connect(
        &server,
        &WaveformServer::protocolError,
        &application,
        [](const QString &message) {
            std::cerr
                << "Protocol error: "
                << message.toStdString()
                << std::endl;
        });

    if (!server.listen()) {
        std::cerr
            << "Cannot listen on 127.0.0.1:45454"
            << std::endl;
        return 1;
    }

    QTimer refreshTimer;

    QObject::connect(
        &refreshTimer,
        &QTimer::timeout,
        &application,
        [&] {
            if (displaysDirty) {
                rootWidget.refreshDisplays();
                displaysDirty = false;
            }

            rootWidget.processEvents();
        });

    refreshTimer.start(50);

    return application.exec();
}