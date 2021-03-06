#include "myROOTUtils.h"

using namespace std;

Bool_t fexists(string fName){
    if(FILE *file = fopen(fName.c_str(), "r")){
        fclose(file);
        return true;
    }
    else
        return false;
}

void drawNicePlot(TH1 *h, TCanvas *ccNP, TString title, TString xLabel, TString yLabel, Bool_t drawLegend)
{
    ccNP->cd();
    h->SetTitle(title);
    h->GetXaxis()->SetTitle(xLabel);
    h->GetYaxis()->SetTitle(yLabel);
    h->Draw();

    TPad *grid = new TPad("grid","",0,0,1,1);
    grid->Draw();
    grid->cd();
    grid->SetGrid();
}
