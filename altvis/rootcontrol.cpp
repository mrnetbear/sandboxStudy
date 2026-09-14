#include "rootcontrol.h"

#include <TAxis.h>
#include <TROOT.h>

#include <algorithm>
#include <numeric>

TApplication *gApplication = nullptr;

RootWidget::RootWidget()
{
    static int argc = 1;
    static char appName[] = "rootVis";
    static char *argv[] = {appName, nullptr};
    gApplication = new TApplication("rootVis", &argc, argv);
    gROOT->SetBatch(false);

    setupCanvases();
    energyHist = new TH1F("energySpectrum", "Energy spectrum;Baseline-subtracted integral;Counts",
                          2048, 0.0, 1.0e6);
    energyHist->SetLineColor(kBlue + 1);
    energyHist->SetFillColor(kAzure - 9);
    lastRateUpdate = std::chrono::steady_clock::now();
}

RootWidget::~RootWidget()
{
    shutdownROOT();
}

void RootWidget::setupCanvases()
{
    waveformCanvas = new TCanvas("waveformCanvas", "Waveform", 100, 100, 900, 450);
    waveformCanvas->SetGrid();
    energyCanvas = new TCanvas("energyCanvas", "Energy spectrum", 1030, 100, 700, 450);
    energyCanvas->SetGrid();
    rateCanvas = new TCanvas("rateCanvas", "Rate", 100, 590, 900, 320);
    rateCanvas->SetGrid();
}

void RootWidget::setGateRange(int start, int end)
{
    gateStart = std::max(0, start);
    gateEnd = std::max(gateStart + 1, end);
}

double RootWidget::calculateBaseline(const std::vector<double> &samples) const
{
    const size_t baselineEnd = std::min(samples.size(), static_cast<size_t>(std::max(1, gateStart)));
    if (baselineEnd == 0)
        return 0.0;
    return std::accumulate(samples.begin(), samples.begin() + baselineEnd, 0.0) /
           static_cast<double>(baselineEnd);
}

double RootWidget::calculateEnergy(const std::vector<double> &samples) const
{
    const int begin = std::clamp(gateStart, 0, static_cast<int>(samples.size()));
    const int end = std::clamp(gateEnd, begin, static_cast<int>(samples.size()));
    const double baseline = calculateBaseline(samples);

    double energy = 0.0;
    for (int index = begin; index < end; ++index)
        energy += samples[static_cast<size_t>(index)] - baseline;
    return energy;
}

void RootWidget::processWaveform(const std::vector<double> &samples, quint64,
                                 quint16, quint16)
{
    if (samples.empty())
        return;

    currentWaveform = samples;
    energyHist->Fill(calculateEnergy(currentWaveform));
    ++eventsThisSecond;

    const auto now = std::chrono::steady_clock::now();
    if (now - lastRateUpdate >= std::chrono::seconds(1)) {
        rateHistory.push_back(eventsThisSecond);
        eventsThisSecond = 0;
        lastRateUpdate = now;
        while (rateHistory.size() > 60)
            rateHistory.pop_front();
    }
}

void RootWidget::drawWaveform()
{
    if (!waveformCanvas || currentWaveform.empty())
        return;

    waveformCanvas->cd();
    if (waveformGraph) {
        delete waveformGraph;
        waveformGraph = nullptr;
    }
    delete gateStartLine; gateStartLine = nullptr;
    delete gateEndLine; gateEndLine = nullptr;
    delete gateBox; gateBox = nullptr;

    waveformGraph = new TGraph(static_cast<int>(currentWaveform.size()));
    for (int i = 0; i < static_cast<int>(currentWaveform.size()); ++i)
        waveformGraph->SetPoint(i, i, currentWaveform[static_cast<size_t>(i)]);

    waveformGraph->SetTitle("Latest waveform;Sample;ADC code");
    waveformGraph->SetLineColor(kBlack);
    waveformGraph->Draw("AL");

    const double ymin = waveformGraph->GetYaxis()->GetXmin();
    const double ymax = waveformGraph->GetYaxis()->GetXmax();
    const int left = std::clamp(gateStart, 0, static_cast<int>(currentWaveform.size()));
    const int right = std::clamp(gateEnd, left, static_cast<int>(currentWaveform.size()));

    gateBox = new TBox(left, ymin, right, ymax);
    gateBox->SetFillColorAlpha(kYellow, 0.25);
    gateBox->Draw();
    gateStartLine = new TLine(left, ymin, left, ymax);
    gateEndLine = new TLine(right, ymin, right, ymax);
    gateStartLine->SetLineColor(kRed);
    gateEndLine->SetLineColor(kRed);
    gateStartLine->SetLineStyle(2);
    gateEndLine->SetLineStyle(2);
    gateStartLine->Draw();
    gateEndLine->Draw();
    waveformCanvas->Modified();
    waveformCanvas->Update();
}

void RootWidget::drawSpectrum()
{
    if (!energyCanvas || !energyHist)
        return;
    energyCanvas->cd();
    energyHist->Draw("hist");
    energyCanvas->Modified();
    energyCanvas->Update();
}

void RootWidget::drawRate()
{
    if (!rateCanvas)
        return;
    rateCanvas->cd();
    delete rateGraph;
    rateGraph = new TGraph(static_cast<int>(rateHistory.size()));
    int i = 0;
    for (int rate : rateHistory)
        rateGraph->SetPoint(i, i++, rate);
    rateGraph->SetTitle("Rate;Seconds ago;Events/s");
    rateGraph->SetMarkerStyle(20);
    rateGraph->SetLineColor(kRed + 1);
    rateGraph->Draw("ALP");
    rateCanvas->Modified();
    rateCanvas->Update();
}

void RootWidget::refreshDisplays()
{
    drawWaveform();
    drawSpectrum();
    drawRate();
}

void RootWidget::processEvents()
{
    gSystem->ProcessEvents();
}

void RootWidget::shutdownROOT()
{
    delete gateStartLine; gateStartLine = nullptr;
    delete gateEndLine; gateEndLine = nullptr;
    delete gateBox; gateBox = nullptr;
    delete waveformGraph; waveformGraph = nullptr;
    delete rateGraph; rateGraph = nullptr;
    delete energyHist; energyHist = nullptr;
    delete waveformCanvas; waveformCanvas = nullptr;
    delete energyCanvas; energyCanvas = nullptr;
    delete rateCanvas; rateCanvas = nullptr;
}