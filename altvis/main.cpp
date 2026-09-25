#include "rootcontrol.h"
#include "waveformserver.h"

#include <QCoreApplication>
#include <QTimer>

#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    QCoreApplication qtApplication(argc, argv);

    RootWidget rootWidget;
    rootWidget.setGateRange(5000, 5999);

    WaveformServer server;
    if (!server.listen()) {
        std::cerr << "Cannot listen on 127.0.0.1:45454" << std::endl;
        return 1;
    }

    std::cout << "ROOT visualizer listens on 127.0.0.1:45454" << std::endl;

    QTimer updateTimer;
    QObject::connect(&updateTimer, &QTimer::timeout, [&] {
        DigitizerProtocol::Event event;
        bool received = false;
        while (server.takeNextEvent(event)) {
            std::vector<double> samples;
            samples.reserve(static_cast<size_t>(event.samples.size()));
            for (quint16 sample : event.samples)
                samples.push_back(static_cast<double>(sample));
            rootWidget.processWaveform(samples, event.timestamp, event.board, event.channel);
            received = true;
        }
        if (received)
            rootWidget.refreshDisplays();
        rootWidget.processEvents();
    });
    updateTimer.start(100);

    return qtApplication.exec();
}