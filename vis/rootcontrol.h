#ifndef ROOTCONTROL_H
#define ROOTCONTROL_H

#include <TCanvas.h>
#include <TRootCanvas.h>
#include <TApplication.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TROOT.h>
#include <TSystem.h>
#include <iostream>
#include <time.h>

class RootWidget
{

public:
    explicit RootWidget();
    ~RootWidget();

    void drawHistogram();
    void drawGraph();
    void clearCanvas();
    void shutdownROOT();

protected:
    //void paintEvent(QPaintEvent *event) override;
    //void resizeEvent(QResizeEvent *event) override;

private:
    TCanvas *rootCanvas;
};

#endif // ROOTCONTROL_H
