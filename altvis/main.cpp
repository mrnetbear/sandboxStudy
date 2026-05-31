#include "rootcontrol.h"
#include <thread>
#include <random>
#include <chrono>

// Функция для имитации получения данных с оцифровщика
void simulateDataAcquisition(RootWidget& rootWidget) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> energyDist(0, 4000);
    std::normal_distribution<> waveformDist(500, 100);
    
    int eventCount = 0;
    
    while (eventCount < 10000) {  // Симулируем 1000 событий
        // Генерируем (синусоида с шумом)
        std::vector<double> waveform(1024);
        for (int i = 0; i < 1024; ++i) {
            double t = i * 2 * M_PI / 256;
            waveform[i] = 500 + 400 * sin(t) + waveformDist(gen) * 0.1;
        }
        
        // Отрисовываем
        rootWidget.drawWaveform(waveform, "Real-time Waveform");
        
        // Вычисляем энергию (интеграл в области гейта)
        auto gateRange = rootWidget.getGateRange();
        double energy = 0;
        for (int i = gateRange.first; i < gateRange.second && i < 1024; ++i) {
            energy += waveform[i];
        }
        
        // Добавляем событие в спектр
        double timestamp = eventCount * 0.01;  // 10 мс между событиями
        rootWidget.addEnergyEvent(energy, timestamp);
        
        // Обновляем спектр каждый 10-й раз для производительности
        if (eventCount % 10 == 0) {
            rootWidget.drawEnergySpectrum();
            rootWidget.drawRateTimePlot();
        }
        
        // Обрабатываем события ROOT
        rootWidget.processEvents();
        
        // Имитируем задержку между событиями
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        eventCount++;
    }
    
    std::cout << "Data acquisition simulation completed" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "Starting RootVis application..." << std::endl;
    
    // Создаем главное окно
    RootWidget rootWidget;
    
    // Устанавливаем гейт для (примерно в середине)
    rootWidget.setGateRange(400, 600);
    
    // Запускаем симуляцию сбора данных в отдельном потоке
    std::thread acquisitionThread(simulateDataAcquisition, std::ref(rootWidget));
    
    // Ждем завершения
    acquisitionThread.join();
    
    // Даем пользователю время посмотреть результаты
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}