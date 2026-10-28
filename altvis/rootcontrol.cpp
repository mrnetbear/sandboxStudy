#include "rootcontrol.h"

#include <TAxis.h>
#include <TROOT.h>

#include <algorithm>
#include <numeric>
#include <cmath>


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
                          2048, .0, 1.0e5);
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
        energy += std::abs(samples[static_cast<size_t>(index)] - baseline);
    return energy;
}

void RootWidget::processWaveform(const std::vector<double> &samples, quint64 timestamp,
                                 quint16 board, quint16 channel)
{
    if (samples.empty())
        return;

    if (samples.size() < 2 ||
    channel >= channelGateSettings.size()) {
    return;
    }

    currentWaveform = samples;
    displayedChannel = channel;
    hasDisplayedWaveform = true;

    updateGateForWaveform(
        channel,
        static_cast<int>(samples.size()));

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
    //gateBox->Draw();
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

void RootWidget::updateGateForWaveform(
    quint16 channel,
    int sampleCount)
{
    if (channel >= channelGateSettings.size() ||
        sampleCount < 2) {
        gateStart = 0;
        gateEnd = 0;
        return;
    }

    const GateSettings &settings =
        channelGateSettings[channel];

    const int widthSamples = std::clamp(
        static_cast<int>(std::lround(
            sampleCount * settings.widthPercent / 100.0)),
        2,
        sampleCount);

    const double center =
        sampleCount * settings.positionPercent / 100.0;

    const int requestedStart =
        static_cast<int>(std::lround(
            center - widthSamples / 2.0));

    gateStart = std::clamp(
        requestedStart,
        0,
        sampleCount - widthSamples);

    gateEnd = gateStart + widthSamples;
}

bool RootWidget::applyGateSettings(
    quint16 channel,
    quint16 widthPercent,
    quint16 positionPercent)
{
    if (channel >= channelGateSettings.size() ||
        widthPercent < 1 ||
        widthPercent > 100 ||
        positionPercent > 100) {
        return false;
    }

    GateSettings &settings =
        channelGateSettings[channel];

    const bool changed =
        settings.widthPercent != widthPercent ||
        settings.positionPercent != positionPercent;

    settings.widthPercent = widthPercent;
    settings.positionPercent = positionPercent;

    // Повторная отправка той же конфигурации, например
    // после переподключения, не должна очищать спектр.
    if (!changed)
        return true;

    // В текущем варианте гистограмма одна.
    // Не смешиваем интегралы с разными настройками гейта.
    if (energyHist)
        energyHist->Reset();

    if (hasDisplayedWaveform &&
        displayedChannel == channel &&
        !currentWaveform.empty()) {

        updateGateForWaveform(
            channel,
            static_cast<int>(currentWaveform.size()));

        // Только перерисовка: не добавляем последнее
        // событие повторно в энергетический спектр.
        drawWaveform();
    }

    drawSpectrum();

    return true;
}

void RootWidget::drawSpectrum()
{
    if (!energyCanvas || !energyHist)
        return;
    energyCanvas->cd();
    energyHist->Draw("hist");
    energyCanvas->Modified();
    energyCanvas->Update();
    TFile *oGrph = new TFile("Spectrum.root", "RECREATE");
    energyHist->Write();
    oGrph->Close();
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

void RootWidget::updateGateIndices(int sampleCount)
{
    if (sampleCount < 2) {
        gateStart = 0;
        gateEnd = sampleCount;
        return;
    }

    const int width = std::clamp(
        static_cast<int>(std::lround(
            gateWidthPercent * sampleCount / 100.0)),
        2,
        sampleCount);

    const double center =
        gatePositionPercent * sampleCount / 100.0;

    const int requestedStart = static_cast<int>(
        std::lround(center - width / 2.0));

    gateStart = std::clamp(
        requestedStart, 0, sampleCount - width);

    gateEnd = gateStart + width;
}

bool RootWidget::setGatePercent(
    double positionPercent, double widthPercent)
{
    if (!std::isfinite(positionPercent) ||
        !std::isfinite(widthPercent) ||
        positionPercent < 0.0 ||
        positionPercent > 100.0 ||
        widthPercent < 1.0 ||
        widthPercent > 100.0) {
        return false;
    }

    if (gatePositionPercent == positionPercent &&
        gateWidthPercent == widthPercent) {
        return true;
    }

    gatePositionPercent = positionPercent;
    gateWidthPercent = widthPercent;

    // Не смешиваем интегралы, полученные с разными окнами.
    if (energyHist)
        energyHist->Reset();

    updateGateIndices(
        static_cast<int>(currentWaveform.size()));

    return true;
}

void RootWidget::clearForChannelChange()
{
    currentWaveform.clear();

    if (energyHist)
        energyHist->Reset();

    rateHistory.clear();
    eventsThisSecond = 0;
    lastRateUpdate = std::chrono::steady_clock::now();

    // Убираем изображение предыдущего канала.
    if (waveformCanvas) {
        waveformCanvas->Clear();
        waveformCanvas->Modified();
        waveformCanvas->Update();
    }
}