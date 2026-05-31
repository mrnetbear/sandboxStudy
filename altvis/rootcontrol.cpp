#include "rootcontrol.h"
#include <TGMsgBox.h>

extern TApplication *gApplication;

// Глобальный указатель для колбэков
static RootWidget* g_rootWidget = nullptr;

RootWidget::RootWidget()
    : waveformCanvas(nullptr)
    , energyCanvas(nullptr)
    , rateCanvas(nullptr)
    , combinedCanvas(nullptr)
    , waveformGraph(nullptr)
    , gateArea(nullptr)
    , energyHist(nullptr)
    , rateGraph(nullptr)
    , multiGraph(nullptr)
    , gateStart(0)
    , gateEnd(0)
    , gateStartLine(-1)
    , gateEndLine(-1)
    , eventsInCurrentSecond(0)
{
    // Инициализируем ROOT
    static int argc = 1;
    static char *argv[] = {(char*)"rootVis"};
    
    if (!gApplication) {
        gApplication = new TApplication("RootVis App", &argc, argv);
        gROOT->SetWebDisplay("off");
        gROOT->SetStyle("Modern");  // Современный стиль
    }
    
    g_rootWidget = this;
    
    // Создаем canvas'ы
    setupCanvases();
    
    // Инициализируем гистограммы
    energyHist = new TH1F("energySpectrum", "Energy Spectrum;Energy (a.u.);Counts", 1024, 0, 4096);
    energyHist->SetFillColor(kBlue-9);
    energyHist->SetLineColor(kBlue);
    energyHist->SetLineWidth(2);
    
    rateGraph = new TGraph();
    rateGraph->SetMarkerStyle(20);
    rateGraph->SetMarkerSize(0.8);
    rateGraph->SetMarkerColor(kRed);
    rateGraph->SetLineColor(kRed);
    rateGraph->SetLineWidth(2);
    
    waveformGraph = new TGraph();
    waveformGraph->SetLineColor(kBlack);
    waveformGraph->SetLineWidth(2);
    
    lastRateUpdate = std::chrono::steady_clock::now();
}

RootWidget::~RootWidget() {
    shutdownROOT();
    std::cout << "ROOT widget destroyed" << std::endl;
}

void RootWidget::setupCanvases() {
    // Canvas для波形
    waveformCanvas = new TCanvas("waveformCanvas", "Waveform Display", 100, 100, 800, 400);
    waveformCanvas->SetGrid();
    waveformCanvas->SetFillColor(kWhite);
    
    // Canvas для энергетического спектра
    energyCanvas = new TCanvas("energyCanvas", "Energy Spectrum", 920, 100, 600, 400);
    energyCanvas->SetLogy();  // Логарифмическая шкала для спектра
    energyCanvas->SetGrid();
    
    // Canvas для Rate-time
    rateCanvas = new TCanvas("rateCanvas", "Rate vs Time", 100, 520, 800, 300);
    rateCanvas->SetGrid();
    
    // Объединенный canvas (опционально)
    combinedCanvas = new TCanvas("combinedCanvas", "Combined View", 920, 520, 600, 300);
    
    // Настраиваем колбэки для закрытия окон
    if (waveformCanvas->GetCanvasImp()) {
        TRootCanvas *imp = (TRootCanvas*)waveformCanvas->GetCanvasImp();
        imp->Connect("CloseWindow()", "RootWidget", this, "handleCanvasClosed(TRootCanvas*)");
    }
}

void RootWidget::drawWaveform(const std::vector<double>& samples, const std::string& title) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    currentWaveform = samples;
    updateWaveformDisplay();
    
    if (waveformCanvas) {
        waveformCanvas->SetTitle(title.c_str());
        waveformCanvas->Update();
    }
}

void RootWidget::drawWaveformWithGate(const std::vector<double>& samples, int gateStart, int gateEnd) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    currentWaveform = samples;
    this->gateStart = gateStart;
    this->gateEnd = gateEnd;
    
    updateWaveformDisplay();
    drawGateOnWaveform();
    
    if (waveformCanvas) {
        waveformCanvas->Update();
    }
}

void RootWidget::updateWaveformDisplay() {
    if (!waveformCanvas) return;
    
    waveformCanvas->cd();
    
    // Очищаем существующий график
    if (waveformGraph) {
        delete waveformGraph;
    }
    
    // Создаем новый график
    waveformGraph = new TGraph();
    for (size_t i = 0; i < currentWaveform.size(); ++i) {
        waveformGraph->SetPoint(i, i, currentWaveform[i]);
    }
    
    waveformGraph->SetTitle("Waveform;Sample Number;Amplitude (ADC)");
    waveformGraph->SetLineColor(kBlack);
    waveformGraph->SetLineWidth(2);
    waveformGraph->Draw("AL");
    
    // Настраиваем оси
    waveformGraph->GetXaxis()->SetRangeUser(0, currentWaveform.size());
    waveformGraph->GetYaxis()->SetTitleOffset(1.2);
    
    // Добавляем легенду
    auto legend = new TLegend(0.85, 0.85, 0.98, 0.98);
    legend->AddEntry(waveformGraph, "Waveform", "l");
    legend->Draw();
}

void RootWidget::drawGateOnWaveform() {
    if (!waveformCanvas) return;
    
    removeGateLines();
    
    waveformCanvas->cd();
    
    // Рисуем вертикальные линии для гейта
    auto startLine = new TLine(gateStart, 
                                  waveformGraph->GetYaxis()->GetXmin(),
                                  gateStart,
                                  waveformGraph->GetYaxis()->GetXmax());
    startLine->SetLineColor(kRed);
    startLine->SetLineWidth(3);
    startLine->SetLineStyle(2);
    startLine->Draw();
    gateStartLine = startLine->GetUniqueID();
    
    TLine *endLine = new TLine(gateEnd,
                                waveformGraph->GetYaxis()->GetXmin(),
                                gateEnd,
                                waveformGraph->GetYaxis()->GetXmax());
    endLine->SetLineColor(kRed);
    endLine->SetLineWidth(3);
    endLine->SetLineStyle(2);
    endLine->Draw();
    gateEndLine = endLine->GetUniqueID();
    
    // Закрашиваем область гейта
    if (gateStart < gateEnd) {
        TBox *gateBox = new TBox(gateStart, 
                                  waveformGraph->GetYaxis()->GetXmin(),
                                  gateEnd,
                                  waveformGraph->GetYaxis()->GetXmax());
        gateBox->SetFillColor(kYellow);
        gateBox->SetFillStyle(3001);  // Прозрачная заливка
        gateBox->Draw();
    }
    
    waveformCanvas->Update();
}

void RootWidget::removeGateLines() {
    if (waveformCanvas) {
        TList *primitives = waveformCanvas->GetListOfPrimitives();
        // Удаляем старые линии (упрощенная версия)
        // В реальном коде нужно хранить указатели и удалять их
    }
}

void RootWidget::addEnergyEvent(double energy, double timestamp) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    // Добавляем событие в спектр
    energyEvents.push_back(energy);
    if (energyHist) {
        energyHist->Fill(energy);
    }
    
    // Обновляем счетчик Rate
    eventsInCurrentSecond++;
    calculateRate();
    
    // Добавляем точку Rate
    ratePoints.push_back({timestamp, eventsInCurrentSecond});
    
    // Ограничиваем размер очередей
    const size_t MAX_EVENTS = 10000;
    while (energyEvents.size() > MAX_EVENTS) {
        energyEvents.pop_front();
    }
    while (ratePoints.size() > 1000) {
        ratePoints.pop_front();
    }
}

void RootWidget::addRatePoint(double timestamp) {
    std::lock_guard<std::mutex> lock(dataMutex);
    calculateRate();
    updateRateGraph();
}

void RootWidget::calculateRate() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRateUpdate);
    
    if (elapsed.count() >= 1) {
        // Каждую секунду обновляем Rate
        rateHistory.push_back(eventsInCurrentSecond);
        eventsInCurrentSecond = 0;
        lastRateUpdate = now;
        
        // Ограничиваем историю (последние 60 секунд)
        while (rateHistory.size() > 60) {
            rateHistory.pop_front();
        }
        
        updateRateGraph();
    }
}

void RootWidget::updateRateGraph() {
    if (!rateCanvas || !rateGraph) return;
    
    rateCanvas->cd();
    
    // Обновляем график
    delete rateGraph;
    rateGraph = new TGraph();
    
    double currentTime = 0;
    for (size_t i = 0; i < rateHistory.size(); ++i) {
        rateGraph->SetPoint(i, currentTime, rateHistory[i]);
        currentTime += 1.0;
    }
    
    rateGraph->SetTitle("Rate vs Time;Time (s);Rate (events/s)");
    rateGraph->SetMarkerStyle(20);
    rateGraph->SetMarkerSize(0.8);
    rateGraph->SetMarkerColor(kRed);
    rateGraph->SetLineColor(kRed);
    rateGraph->SetLineWidth(2);
    rateGraph->Draw("ALP");
    
    rateCanvas->Update();
}

void RootWidget::updateSpectra() {
    if (!energyCanvas) return;
    
    energyCanvas->cd();
    
    if (energyHist) {
        energyHist->Draw("hist");
        energyCanvas->Update();
    }
}

void RootWidget::drawEnergySpectrum() {
    if (energyCanvas) {
        energyCanvas->cd();
        
        // Нормализуем гистограмму, если нужно
        // energyHist->Scale(1.0 / energyHist->Integral());
        
        energyHist->Draw("hist");
        energyCanvas->Update();
    }
}

void RootWidget::drawRateTimePlot() {
    updateRateGraph();
}

void RootWidget::drawCombinedView() {
    if (combinedCanvas) {
        combinedCanvas->Divide(2, 1);
        
        combinedCanvas->cd(1);
        if (energyHist) {
            energyHist->Draw("hist");
        }
        
        combinedCanvas->cd(2);
        if (rateGraph) {
            rateGraph->Draw("ALP");
        }
        
        combinedCanvas->Update();
    }
}

void RootWidget::updateWaveform(const std::vector<double>& samples) {
    drawWaveform(samples, "Real-time Waveform");
}

void RootWidget::setGateRange(int start, int end) {
    gateStart = start;
    gateEnd = end;
    
    if (!currentWaveform.empty()) {
        drawWaveformWithGate(currentWaveform, gateStart, gateEnd);
    }
}

void RootWidget::clearAll() {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    if (energyHist) {
        energyHist->Reset();
    }
    
    energyEvents.clear();
    rateHistory.clear();
    ratePoints.clear();
    eventsInCurrentSecond = 0;
    
    if (waveformGraph) {
        waveformGraph->SetPoint(0, 0, 0);
    }
    
    updateSpectra();
    updateRateGraph();
    
    std::cout << "All displays cleared" << std::endl;
}

void RootWidget::processEvents() {
    if (gApplication) {
        // gApplication->HandleExceptions();
        gSystem->ProcessEvents();
    }
}

void RootWidget::shutdownROOT() {
    std::cout << "Shutting down ROOT..." << std::endl;
    
    if (waveformCanvas) {
        delete waveformCanvas;
        waveformCanvas = nullptr;
    }
    
    if (energyCanvas) {
        delete energyCanvas;
        energyCanvas = nullptr;
    }
    
    if (rateCanvas) {
        delete rateCanvas;
        rateCanvas = nullptr;
    }
    
    if (combinedCanvas) {
        delete combinedCanvas;
        combinedCanvas = nullptr;
    }
    
    if (energyHist) {
        delete energyHist;
        energyHist = nullptr;
    }
    
    if (rateGraph) {
        delete rateGraph;
        rateGraph = nullptr;
    }
    
    if (waveformGraph) {
        delete waveformGraph;
        waveformGraph = nullptr;
    }
    
    std::cout << "ROOT shutdown complete" << std::endl;
}

void RootWidget::handleCanvasClosed(TRootCanvas *canvas) {
    std::cout << "Canvas closed" << std::endl;
    if (gApplication) {
        // Не завершаем приложение полностью, просто уведомляем
        // gApplication->Terminate();
    }
}