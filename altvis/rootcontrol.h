#ifndef ROOTCONTROL_H
#define ROOTCONTROL_H

#include <QtGlobal>

#include <TBox.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TLine.h>

#include <chrono>
#include <deque>
#include <string>
#include <utility>
#include <vector>
#include <array>

class RootWidget
{
public:
    RootWidget();
    ~RootWidget();

    void processWaveform(const std::vector<double> &samples,
                         quint64 timestamp,
                         quint16 board,
                         quint16 channel);

    void refreshDisplays();
    void processEvents();

    void setIntegrationRanges(int baselineStart,
                              int baselineEnd,
                              int gateStart,
                              int gateEnd);

    std::array<int, 4> integrationRanges() const;

    void setNegativePulse(bool enabled);
    bool negativePulse() const;

    bool saveEnergySpectrum(const std::string &rootFileName,
                            const std::string &csvFileName) const;

    void shutdownROOT();

private:
    void setupCanvases();

    void drawWaveform();
    void drawSpectrum();
    void drawRate();

    double calculateBaseline(const std::vector<double> &samples) const;
    double calculateEnergyTrapezoidal(
        const std::vector<double> &samples) const;

    TCanvas *waveformCanvas = nullptr;
    TCanvas *energyCanvas = nullptr;
    TCanvas *rateCanvas = nullptr;

    TGraph *waveformGraph = nullptr;
    TGraph *rateGraph = nullptr;

    TH1F *energyHist = nullptr;

    TLine *baselineStartLine = nullptr;
    TLine *baselineEndLine = nullptr;
    TLine *gateStartLine = nullptr;
    TLine *gateEndLine = nullptr;

    TBox *baselineBox = nullptr;
    TBox *gateBox = nullptr;

    std::vector<double> currentWaveform;

    quint64 currentEventId = 0;
    quint64 currentTimestamp = 0;
    quint16 currentBoard = 0;
    quint16 currentChannel = 0;

    int baselineStart = 0;
    int baselineEnd = 200;

    int gateStart = 400;
    int gateEnd = 600;

    bool negativePulseMode = false;

    std::deque<int> rateHistory;
    int eventsThisSecond = 0;

    std::chrono::steady_clock::time_point lastRateUpdate;
};

#endif // ROOTCONTROL_H