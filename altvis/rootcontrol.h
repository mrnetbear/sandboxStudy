#ifndef ROOTCONTROL_H
#define ROOTCONTROL_H

#include <QtGlobal>

#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TLine.h>
#include <TBox.h>
#include <TSystem.h>

#include <chrono>
#include <deque>
#include <string>
#include <utility>
#include <vector>

class RootWidget
{
public:
    RootWidget();
    ~RootWidget();

    void processWaveform(const std::vector<double> &samples, quint64 timestamp,
                         quint16 board, quint16 channel);
    void refreshDisplays();
    void processEvents();

    void setGateRange(int start, int end);
    std::pair<int, int> getGateRange() const { return {gateStart, gateEnd}; }
    void shutdownROOT();

private:
    void setupCanvases();
    void drawWaveform();
    void drawSpectrum();
    void drawRate();
    double calculateBaseline(const std::vector<double> &samples) const;
    double calculateEnergy(const std::vector<double> &samples) const;

    TCanvas *waveformCanvas = nullptr;
    TCanvas *energyCanvas = nullptr;
    TCanvas *rateCanvas = nullptr;
    TGraph *waveformGraph = nullptr;
    TGraph *rateGraph = nullptr;
    TH1F *energyHist = nullptr;
    TLine *gateStartLine = nullptr;
    TLine *gateEndLine = nullptr;
    TBox *gateBox = nullptr;

    std::vector<double> currentWaveform;
    int gateStart = 400;
    int gateEnd = 600;
    std::deque<int> rateHistory;
    int eventsThisSecond = 0;
    std::chrono::steady_clock::time_point lastRateUpdate;
};

#endif // ROOTCONTROL_H