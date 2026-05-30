#include "rootcontrol.h"
extern TApplication *gApplication;

RootWidget::RootWidget()
    : rootCanvas(nullptr)
{
    // Инициализируем ROOT, если еще не инициализирован
    static int argc = 1;
    static char *argv[] = {(char*)"myapp"};

    if (!gApplication) {
        gApplication = new TApplication("My ROOT App", &argc, argv);
        gROOT->SetWebDisplay("off");
    }
}

RootWidget::~RootWidget(){
    shutdownROOT();
    std::cout << "Завершение ROOT успешно..." << std::endl;

}

void RootWidget::drawHistogram()
{
    if (rootCanvas) {
        rootCanvas->Clear();
    } else {
        rootCanvas = new TCanvas("myCanvas", "ROOT Canvas", 800, 600);
    }

    rootCanvas->cd();

    // Создаем и рисуем гистограмму
    TH1F *hist = new TH1F("hist", "Histogram Example", 100, -5, 5);
    for (size_t i = 0; i < 100; ++i){
    hist->FillRandom("gaus", 10000);
        hist->SetFillColor(kBlue);
        hist->Draw();
        rootCanvas->Modified();
        rootCanvas->Update();
        TRootCanvas *tRootCanvas = (TRootCanvas *)rootCanvas->GetCanvasImp();
        tRootCanvas->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
        sleep(1);
    }


    if (gApplication) {
            gApplication->Run(); // kTRUE = один раз обработать события
        }
    
}

void RootWidget::drawGraph()
{
    if (rootCanvas) {
        rootCanvas->Clear();
    } else {
        rootCanvas = new TCanvas("myCanvas", "ROOT Canvas", 800, 600);
    }

    rootCanvas->cd();

    // Создаем и рисуем график
    TGraph *graph = new TGraph();
    for (int i = 0; i < 10; i++) {
        graph->SetPoint(i, i, i*i);
    }
    graph->SetMarkerStyle(20);
    graph->SetMarkerColor(kRed);
    graph->Draw("AP");

    rootCanvas->Update();
}

void RootWidget::clearCanvas()
{
    if (rootCanvas) {
        rootCanvas->Clear();
        rootCanvas->Update();
    }
}

void RootWidget::shutdownROOT()
{
    std::cout << "Завершение ROOT..." << std::endl;

    if (rootCanvas) {
        delete rootCanvas;
        rootCanvas = nullptr;
    }

    std::cout << "ROOTCanvas deleted..." << std::endl;
}
