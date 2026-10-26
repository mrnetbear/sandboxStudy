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
#include <TFile.h>

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

    bool setGatePercent(double positionPercent,
                    double widthPercent);

    void clearForChannelChange();

    void setGateRange(int start, int end);
    std::pair<int, int> getGateRange() const { return {gateStart, gateEnd}; }
    void shutdownROOT();

private:
    void setupCanvases();
    void drawWaveform();
    void drawSpectrum();
    void drawRate();
    
    void updateGateIndices(int sampleCount);

    double gatePositionPercent = 50.0;
    double gateWidthPercent = 50.0;

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
    int gateStart = 5000;
    int gateEnd = 10000;
    std::deque<int> rateHistory;
    int eventsThisSecond = 0;
    std::chrono::steady_clock::time_point lastRateUpdate;
};

#endif // ROOTCONTROL_H