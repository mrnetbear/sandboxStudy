#include "rootcontrol.h"

#include <TApplication.h>
#include <TAxis.h>
#include <TFile.h>
#include <TNamed.h>
#include <TROOT.h>
#include <TSystem.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

namespace {
TApplication *gRootApplication = nullptr;

constexpr double SamplePeriod = 1.0;
// Сейчас результат имеет единицы ADC-code * sample.
// Если известен период дискретизации платы в ns,
// например 2.0 ns, замени 1.0 на 2.0.
}

RootWidget::RootWidget()
{
    if (!gRootApplication) {
        static int rootArgc = 1;
        static char appName[] = "rootVis";
        static char *rootArgv[] = {appName, nullptr};

        gRootApplication = new TApplication(
            "rootVis",
            &rootArgc,
            rootArgv
        );
    }

    gROOT->SetBatch(false);

    setupCanvases();

    energyHist = new TH1F(
        "energySpectrum",
        "Energy spectrum;Integrated charge (ADC #upoint sample);Counts",
        4096,
        -1.0e6,
        1.0e6
    );

    energyHist->SetLineColor(kBlue + 1);
    energyHist->SetFillColor(kAzure - 9);
    energyHist->SetLineWidth(2);

    lastRateUpdate = std::chrono::steady_clock::now();
}

RootWidget::~RootWidget()
{
    shutdownROOT();
}

void RootWidget::setupCanvases()
{
    waveformCanvas = new TCanvas(
        "waveformCanvas",
        "Waveform",
        100,
        100,
        1000,
        500
    );

    waveformCanvas->SetGrid();

    energyCanvas = new TCanvas(
        "energyCanvas",
        "Energy spectrum",
        1140,
        100,
        750,
        500
    );

    energyCanvas->SetGrid();
    energyCanvas->SetLogy();

    rateCanvas = new TCanvas(
        "rateCanvas",
        "Event rate",
        100,
        650,
        1000,
        330
    );

    rateCanvas->SetGrid();
}

void RootWidget::setIntegrationRanges(int newBaselineStart,
                                      int newBaselineEnd,
                                      int newGateStart,
                                      int newGateEnd)
{
    baselineStart = std::max(0, newBaselineStart);
    baselineEnd = std::max(baselineStart + 1, newBaselineEnd);

    gateStart = std::max(0, newGateStart);
    gateEnd = std::max(gateStart + 1, newGateEnd);
}

std::array<int, 4> RootWidget::integrationRanges() const
{
    return {baselineStart, baselineEnd, gateStart, gateEnd};
}

void RootWidget::setNegativePulse(bool enabled)
{
    negativePulseMode = enabled;
}

bool RootWidget::negativePulse() const
{
    return negativePulseMode;
}

double RootWidget::calculateBaseline(
    const std::vector<double> &samples) const
{
    if (samples.empty())
        return 0.0;

    const int size = static_cast<int>(samples.size());

    const int first = std::clamp(baselineStart, 0, size - 1);
    const int last = std::clamp(baselineEnd, first + 1, size);

    const auto beginIt = samples.begin() + first;
    const auto endIt = samples.begin() + last;

    const double sum = std::accumulate(beginIt, endIt, 0.0);

    return sum / static_cast<double>(last - first);
}

double RootWidget::calculateEnergyTrapezoidal(
    const std::vector<double> &samples) const
{
    if (samples.size() < 2)
        return 0.0;

    const int size = static_cast<int>(samples.size());

    const int first = std::clamp(gateStart, 0, size - 2);
    const int last = std::clamp(gateEnd, first + 1, size - 1);

    const double baseline = calculateBaseline(samples);

    const auto corrected = [this, baseline](double value) {
        return negativePulseMode ? baseline - value : value - baseline;
    };

    double integral =
        0.5 * (
            corrected(samples[static_cast<size_t>(first)]) +
            corrected(samples[static_cast<size_t>(last)])
        );

    for (int sampleIndex = first + 1;
         sampleIndex < last;
         ++sampleIndex) {
        integral += corrected(
            samples[static_cast<size_t>(sampleIndex)]
        );
    }

    return integral * SamplePeriod;
}

void RootWidget::processWaveform(const std::vector<double> &samples,
                                 quint64 timestamp,
                                 quint16 board,
                                 quint16 channel)
{
    if (samples.empty())
        return;

    currentWaveform = samples;
    currentTimestamp = timestamp;
    currentBoard = board;
    currentChannel = channel;
    ++currentEventId;

    const double energy = calculateEnergyTrapezoidal(currentWaveform);

    if (energyHist)
        energyHist->Fill(energy);

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
    waveformCanvas->Clear();

    delete waveformGraph;
    waveformGraph = nullptr;

    delete baselineStartLine;
    baselineStartLine = nullptr;

    delete baselineEndLine;
    baselineEndLine = nullptr;

    delete gateStartLine;
    gateStartLine = nullptr;

    delete gateEndLine;
    gateEndLine = nullptr;

    delete baselineBox;
    baselineBox = nullptr;

    delete gateBox;
    gateBox = nullptr;

    waveformGraph = new TGraph(
        static_cast<int>(currentWaveform.size())
    );

    for (int index = 0;
         index < static_cast<int>(currentWaveform.size());
         ++index) {
        waveformGraph->SetPoint(
            index,
            index,
            currentWaveform[static_cast<size_t>(index)]
        );
    }

    std::ostringstream title;

    title << "Waveform: board " << currentBoard
          << ", channel " << currentChannel
          << ", event " << currentEventId
          << ";Sample;ADC code";

    waveformGraph->SetTitle(title.str().c_str());
    waveformGraph->SetLineColor(kBlack);
    waveformGraph->SetLineWidth(2);
    waveformGraph->Draw("AL");

    waveformCanvas->Update();

    const double yMin = waveformGraph->GetYaxis()->GetXmin();
    const double yMax = waveformGraph->GetYaxis()->GetXmax();

    const int size = static_cast<int>(currentWaveform.size());

    const int baselineLeft = std::clamp(baselineStart, 0, size);
    const int baselineRight = std::clamp(
        baselineEnd,
        baselineLeft,
        size
    );

    const int gateLeft = std::clamp(gateStart, 0, size);
    const int gateRight = std::clamp(gateEnd, gateLeft, size);

    baselineBox = new TBox(
        baselineLeft,
        yMin,
        baselineRight,
        yMax
    );

    baselineBox->SetFillColorAlpha(kGreen + 1, 0.16);
    baselineBox->SetLineColor(kGreen + 2);
    baselineBox->SetLineStyle(2);
    baselineBox->Draw();

    gateBox = new TBox(
        gateLeft,
        yMin,
        gateRight,
        yMax
    );

    gateBox->SetFillColorAlpha(kYellow, 0.24);
    gateBox->SetLineColor(kOrange + 7);
    gateBox->SetLineStyle(2);
    gateBox->Draw();

    baselineStartLine = new TLine(
        baselineLeft,
        yMin,
        baselineLeft,
        yMax
    );

    baselineEndLine = new TLine(
        baselineRight,
        yMin,
        baselineRight,
        yMax
    );

    gateStartLine = new TLine(
        gateLeft,
        yMin,
        gateLeft,
        yMax
    );

    gateEndLine = new TLine(
        gateRight,
        yMin,
        gateRight,
        yMax
    );

    baselineStartLine->SetLineColor(kGreen + 3);
    baselineEndLine->SetLineColor(kGreen + 3);
    gateStartLine->SetLineColor(kRed);
    gateEndLine->SetLineColor(kRed);

    baselineStartLine->SetLineWidth(2);
    baselineEndLine->SetLineWidth(2);
    gateStartLine->SetLineWidth(2);
    gateEndLine->SetLineWidth(2);

    baselineStartLine->SetLineStyle(2);
    baselineEndLine->SetLineStyle(2);
    gateStartLine->SetLineStyle(2);
    gateEndLine->SetLineStyle(2);

    baselineStartLine->Draw();
    baselineEndLine->Draw();
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
    rateCanvas->Clear();

    delete rateGraph;
    rateGraph = nullptr;

    rateGraph = new TGraph(
        static_cast<int>(rateHistory.size())
    );

    int index = 0;

    for (int rate : rateHistory) {
        rateGraph->SetPoint(index, index, rate);
        ++index;
    }

    rateGraph->SetTitle("Event rate;Time index (s);Events/s");
    rateGraph->SetMarkerStyle(20);
    rateGraph->SetMarkerColor(kRed + 1);
    rateGraph->SetLineColor(kRed + 1);
    rateGraph->SetLineWidth(2);
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

bool RootWidget::saveEnergySpectrum(
    const std::string &rootFileName,
    const std::string &csvFileName) const
{
    if (!energyHist)
        return false;

    {
        TFile output(rootFileName.c_str(), "RECREATE");

        if (output.IsZombie())
            return false;

        TH1F *spectrumCopy =
            static_cast<TH1F *>(
                energyHist->Clone("energySpectrum")
            );

        spectrumCopy->SetDirectory(&output);
        spectrumCopy->Write();

        std::ostringstream metadata;

        metadata << "baselineStart=" << baselineStart
                 << ";baselineEnd=" << baselineEnd
                 << ";gateStart=" << gateStart
                 << ";gateEnd=" << gateEnd
                 << ";polarity="
                 << (negativePulseMode ? "negative" : "positive")
                 << ";integration=trapezoidal"
                 << ";samplePeriod=" << SamplePeriod;

        TNamed analysisSettings(
            "analysisSettings",
            metadata.str().c_str()
        );

        analysisSettings.Write();

        output.Write();
        output.Close();

        delete spectrumCopy;
    }

    std::ofstream csv(csvFileName);

    if (!csv.is_open())
        return false;

    csv << "bin,center,width,counts\n";
    csv << std::setprecision(12);

    const int binCount = energyHist->GetNbinsX();

    for (int bin = 1; bin <= binCount; ++bin) {
        csv << bin << ','
            << energyHist->GetBinCenter(bin) << ','
            << energyHist->GetBinWidth(bin) << ','
            << energyHist->GetBinContent(bin)
            << '\n';
    }

    csv << "underflow,,,"
        << energyHist->GetBinContent(0)
        << '\n';

    csv << "overflow,,,"
        << energyHist->GetBinContent(binCount + 1)
        << '\n';

    return csv.good();
}

void RootWidget::shutdownROOT()
{
    delete baselineStartLine;
    baselineStartLine = nullptr;

    delete baselineEndLine;
    baselineEndLine = nullptr;

    delete gateStartLine;
    gateStartLine = nullptr;

    delete gateEndLine;
    gateEndLine = nullptr;

    delete baselineBox;
    baselineBox = nullptr;

    delete gateBox;
    gateBox = nullptr;

    delete waveformGraph;
    waveformGraph = nullptr;

    delete rateGraph;
    rateGraph = nullptr;

    delete energyHist;
    energyHist = nullptr;

    delete waveformCanvas;
    waveformCanvas = nullptr;

    delete energyCanvas;
    energyCanvas = nullptr;

    delete rateCanvas;
    rateCanvas = nullptr;
}