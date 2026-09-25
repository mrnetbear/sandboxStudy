#include "rootcontrol.h"
#include "waveformserver.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QTimer>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

int readIntegerArgument(int argc,
                        char *argv[],
                        const char *name,
                        int defaultValue)
{
    for (int index = 1; index + 1 < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == name) {
            bool ok = false;

            const int value =
                QString::fromLocal8Bit(argv[index + 1]).toInt(&ok);

            if (ok)
                return value;
        }
    }

    return defaultValue;
}

bool hasArgument(int argc, char *argv[], const char *name)
{
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == name)
            return true;
    }

    return false;
}

QString readStringArgument(int argc,
                           char *argv[],
                           const char *name,
                           const QString &defaultValue)
{
    for (int index = 1; index + 1 < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == name)
            return QString::fromLocal8Bit(argv[index + 1]);
    }

    return defaultValue;
}

void printUsage()
{
    std::cout
        << "Usage: rootVis [options]\n"
        << "  --port <port>               TCP port, default: 45454\n"
        << "  --baseline-start <sample>   Default: 0\n"
        << "  --baseline-end <sample>     Default: 200\n"
        << "  --gate-start <sample>       Default: 400\n"
        << "  --gate-end <sample>         Default: 600\n"
        << "  --negative                  Analyze negative pulses\n"
        << "  --positive                  Analyze positive pulses\n"
        << "  --output <directory>        Directory for Ctrl+S output\n"
        << "  --help                      Show this help\n";
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication qtApplication(argc, argv);

    if (hasArgument(argc, argv, "--help")) {
        printUsage();
        return 0;
    }

    const int portArgument =
        readIntegerArgument(argc, argv, "--port", 45454);

    if (portArgument < 1 || portArgument > 65535) {
        std::cerr << "Invalid TCP port" << std::endl;
        return 1;
    }

    const int baselineStart =
        readIntegerArgument(argc, argv, "--baseline-start", 0);

    const int baselineEnd =
        readIntegerArgument(argc, argv, "--baseline-end", 200);

    const int gateStart =
        readIntegerArgument(argc, argv, "--gate-start", 400);

    const int gateEnd =
        readIntegerArgument(argc, argv, "--gate-end", 600);

    const bool negativePulse =
        hasArgument(argc, argv, "--negative") &&
        !hasArgument(argc, argv, "--positive");

    QString outputDirectory =
        readStringArgument(argc, argv, "--output", QDir::currentPath());

    QDir outputDir(outputDirectory);

    if (!outputDir.exists() && !QDir().mkpath(outputDirectory)) {
        std::cerr << "Cannot create output directory: "
                  << outputDirectory.toStdString()
                  << std::endl;
        return 1;
    }

    RootWidget rootWidget;

    rootWidget.setIntegrationRanges(
        baselineStart,
        baselineEnd,
        gateStart,
        gateEnd
    );

    rootWidget.setNegativePulse(negativePulse);

    WaveformServer server;

    if (!server.listen(static_cast<quint16>(portArgument))) {
        std::cerr << "Cannot listen on 127.0.0.1:"
                  << portArgument
                  << std::endl;

        return 1;
    }

    std::cout << "ROOT visualizer listens on 127.0.0.1:"
              << portArgument
              << std::endl;

    std::cout << "Baseline: ["
              << baselineStart
              << ", "
              << baselineEnd
              << "), gate: ["
              << gateStart
              << ", "
              << gateEnd
              << "), polarity: "
              << (negativePulse ? "negative" : "positive")
              << std::endl;

    QObject::connect(
        &server,
        &WaveformServer::clientConnected,
        [] {
            std::cout << "Acquisition client connected" << std::endl;
        }
    );

    QObject::connect(
        &server,
        &WaveformServer::clientDisconnected,
        [] {
            std::cout << "Acquisition client disconnected" << std::endl;
        }
    );

    QObject::connect(
        &server,
        &WaveformServer::protocolError,
        [](const QString &message) {
            std::cerr << "Protocol error: "
                      << message.toStdString()
                      << std::endl;
        }
    );

    QTimer updateTimer;

    QObject::connect(
        &updateTimer,
        &QTimer::timeout,
        [&server, &rootWidget] {
            DigitizerProtocol::Event event;

            bool receivedAnyEvent = false;

            while (server.takeNextEvent(event)) {
                std::vector<double> samples;

                samples.reserve(
                    static_cast<size_t>(event.samples.size())
                );

                for (quint16 sample : event.samples)
                    samples.push_back(static_cast<double>(sample));

                rootWidget.processWaveform(
                    samples,
                    event.timestamp,
                    event.board,
                    event.channel
                );

                receivedAnyEvent = true;
            }

            if (receivedAnyEvent)
                rootWidget.refreshDisplays();

            rootWidget.processEvents();
        }
    );

    // GUI обновляется максимум 20 раз/с.
    // При этом все события из очереди участвуют в histogram.
    updateTimer.start(50);

    return qtApplication.exec();
}