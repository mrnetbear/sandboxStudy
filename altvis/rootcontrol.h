#ifndef ROOTCONTROL_H
#define ROOTCONTROL_H

#include <TCanvas.h>
#include <TRootCanvas.h>
#include <TApplication.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TVirtualPad.h>
#include <TGClient.h>
#include <TGWindow.h>
#include <TFrame.h>
#include <TLegend.h>
#include <TLine.h>
#include <vector>
#include <deque>
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>

// Структура для хранения данных события
struct WaveformData {
    std::vector<double> samples;  // сэмплы
    double charge;                 // заряд (интеграл)
    double amplitude;              // амплитуда
    double timestamp;              // временная метка
    int channel;                   // номер канала
};

class RootWidget
{
public:
    explicit RootWidget();
    ~RootWidget();

    // Основные методы отрисовки
    void drawWaveform(const std::vector<double>& samples, const std::string& title = "Waveform");
    void drawWaveformWithGate(const std::vector<double>& samples, int gateStart, int gateEnd);
    void updateWaveform(const std::vector<double>& samples);
    
    // Спектроскопические методы
    void addEnergyEvent(double energy, double timestamp);
    void addRatePoint(double timestamp);
    void updateSpectra();
    
    // Управление отображением
    void drawEnergySpectrum();
    void drawRateTimePlot();
    void drawCombinedView();
    void clearAll();
    
    // Геттеры/сеттеры
    void setGateRange(int start, int end);
    std::pair<int, int> getGateRange() const { return {gateStart, gateEnd}; }
    
    // Управление ROOT
    void processEvents();
    void shutdownROOT();
    
private:
    // Canvas'ы для разных видов отображения
    TCanvas *waveformCanvas;      // Canvas для
    TCanvas *energyCanvas;        // Canvas для энергетического спектра
    TCanvas *rateCanvas;          // Canvas для Rate-time
    TCanvas *combinedCanvas;      // Объединенный canvas
    
    // Графические объекты
    TGraph *waveformGraph;        // График
    TGraph *gateArea;             // Отображение гейта на
    TH1F *energyHist;             // Энергетический спектр (гистограмма)
    TGraph *rateGraph;            // График Rate-time
    TMultiGraph *multiGraph;      // Для нескольких графиков
    
    // Данные
    std::vector<double> currentWaveform;
    std::deque<double> energyEvents;      // Очередь энергий для спектра
    std::deque<std::pair<double, double>> ratePoints;  // (время, Rate)
    
    // Параметры гейта
    int gateStart;
    int gateEnd;
    int gateStartLine;   // ID линии начала гейта
    int gateEndLine;     // ID линии конца гейта
    
    // Для расчета Rate
    std::chrono::steady_clock::time_point lastRateUpdate;
    int eventsInCurrentSecond;
    std::deque<double> rateHistory;  // История Rate за последние N секунд
    
    // Синхронизация
    std::mutex dataMutex;
    
    // Вспомогательные методы
    void setupCanvases();
    void updateWaveformDisplay();
    void updateRateGraph();
    void drawGateOnWaveform();
    void removeGateLines();
    void calculateRate();
    
    // Статические колбэки для GUI
    static void handleCanvasClosed(TRootCanvas *canvas);
    static void handleMouseMove(TVirtualPad *pad, int event, int x, int y, TObject *selected);
};

#endif // ROOTCONTROL_H