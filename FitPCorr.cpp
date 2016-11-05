#include "FitPCorr.h"
#include "myROOTUtils.h"

using namespace std;

FitPCorr::FitPCorr(Float_t eBeam, TClasTool *ct)
{
    kMassP = 0.938;
    thetaMin[0] = 0;
    thetaMin[1] = 16;
    thetaMin[2] = 20;
    thetaMin[3] = 25;
    thetaMax[0] = 16;
    thetaMax[1] = 20;
    thetaMax[2] = 25;
    thetaMax[3] = 90;
    thMin = 16;
    thMax = 34;
    phiMin[0] = -30;
    phiMax[0] = 30;
    for(Int_t i = 1; i < 6; i++){
        phiMin[i] = phiMin[i-1] + 60;
        phiMax[i] = phiMax[i-1] + 60;
    }
    nSect = 6;
    phiBins = 60;
    thBins = 18;
    f1Bins = 20;
    f2Bins = 20;
    f1Min = 0.9;
    f1Max = 1.1;
    f2Min = 0.9;
    f2Max = 1.1;

    this->fCT = ct;
    this->eBeam = eBeam;
    this->nEntries = (Int_t) fCT->GetEntries();

    fId = new TIdentificator(fCT);
}

FitPCorr::~FitPCorr()
{
    delete fCT;
    delete fId;
}

void FitPCorr::fillHists(TString rootfName, Bool_t printGFit = false, Float_t thCut = 16)
{
    Bool_t hasProton;
    Float_t minThetaE = 100;
    Float_t maxThetaE = 0;
    this->thCut = thCut;
    this->rootfName = rootfName;
    TFile *f = new TFile(rootfName, "RECREATE");
    initHists();

    cout << "Number of Events: " << nEntries << endl;

    for(Int_t i = 0; i < nEntries; i++){
        fCT->Next();
        nRowsEVNT = fCT->GetNRows("EVNT");
        hasProton = false;
        if(nRowsEVNT > 1 && isInitElec()){
            if(minThteaE > fId->ThetaLab(0)) minThetaE = fId->ThetaLab(0);
            if(maxThetaE < fId->ThetaLab(0)) maxThetaE = fId->ThetaLab(0);
            if(fId->ThetaLab(0) >= thCut){
                fEVNT = (TEVNTClass*) fCT->GetBankRow("EVNT", 0);
                for(Int_t j = 0; j < nRowsEVNT; j++){
                    hasProton = isPartProton(j);
                    if(hasProton) break;
                }
                if(hasProton && fEVNT->GetZ() < -10){
                    hWextra->Fill(fId->W());
                    if(fId->W() >= 0.8 && fId->W() <= 1.05){
                        hZ->Fill(fEVNT->GetZ());
                        hW->Fill(fId->W());
                        Int_t sec = fId->Sector(0);
                        hF1[sec]->Fill(fId->PhiLab(0), ratioF1(fId->Momentum(0), fId->ThetaLab(0)));
                        hF2[sec]->Fill(fId->ThetaLab(0), ratioF1(fId->Momentum(0), fId->ThetaLab(0)));
                    }
                }
            }

        }
    }

    cout << "Percentage of event pass cuts << " << hW->GetEntries()/nEntries << endl;

    Int_t binContent;
    Int_t totBin;
    Int_t entriesGaus;

    TH1F *hGauss;
    TF1 *fGauss;

    TCanvas *c1 = new TCanvas("c1", "", 1600, 900);

    for(Int_t i = 0; i < nSect; i++){
        for(Int_t j = 0; j < phiBins; j++){
            totBin = 0;
            entriesGaus = 0;
            for(Int_t k = 0; k < f1Bins; k++)
                totBin = totBin + hF1[i]->GetBinContent(j+1, k+1);
            if(totBin == 0) continue;
            hGauss = new TH1F("hGauss", "hGauss", f1Bins, f1Min, f1Max);
            fGauss = new TF1("fGauss", "gaus(0)", 0.95, 1.05);
            for(Int_t k = 0; k < f1Bins; k++){
                binContent = hF1[i]->GetBinContent(j+1, k+1);
                entriesGaus = entriesGaus + binContent;
                if(binContent != 0){
                    hGauss->Fill(hF1[i]->GetYaxis()->GetBinCenter(k+1), binContent);
                }
            }
            if(entriesGaus < 50) continue;
            hGauss->Fit("fGauss", "EQR");
            if(printGFit){
                c1->cd();
                hGauss->Draw();
                c1->SaveAs(Form("fitGaussf1_sec%d_bin%d.png", i+1, j+1));
            }
            hF1Mean[i]->SetBinContent(j+1, fGauss->GetParameter(1));
            hF1Mean[i]->SetBinError(j+1, fGauss->GetParError(1));
            delete hGauss;
            delete fGauss;
        }
    }

    for(Int_t i = 0; i < nSect; i++){
        for(Int_t j = 0; j < thBins; j++){
            totBin = 0;
            entriesGaus = 0;
            for(Int_t k = 0; k < f2Bins; k++)
                totBin = totBin + hF2[i]->GetBinContent(j+1, k+1);
            if(totBin == 0) continue;
            hGauss = new TH1F("hGauss", "hGauss", f2Bins, f2Min, f2Max);
            fGauss = new TF1("fGauss", "gaus(0)", 0.95, 1.05);
            for(Int_t k = 0; k < f2Bins; k++){
                binContent = hF2[i]->GetBinContent(j+1, k+1);
                entriesGaus = entriesGaus + binContent;
                if(binContent != 0){
                    hGauss->Fill(hF2[i]->GetYaxis()->GetBinCenter(k+1), binContent);
                }
            }
            if(entriesGaus < 25) continue;
            hGauss->Fit("fGauss", "EQR");
            if(printGFit){
                c1->cd();
                hGauss->Draw();
                c1->SaveAs(Form("fitGaussf2_sec%d_bin%d.png", i+1, j+1));
            }
            hF2Mean[i]->SetBinContent(j+1, fGauss->GetParameter(1));
            hF2Mean[i]->SetBinError(j+1, fGauss->GetParError(1));
            delete hGauss;
            delete fGauss;
        }
    }

    writeHists(f);
    delete c1;
}

void FitPCorr::fillThHists()
{
}

void FitPCorr::fitHists(Bool_t setInitParams = false, Bool_t tryF2e = false)
{
    TFile *f = new TFile(rootfName, "UPDATE");

    TH1F *hF1r;
    TH1F *hF2r;
    TF1 *f1[nSect];
    TF1 *f2[nSect];
    TF1 *f2e[nSect];

    for(Int_t i = 0; i < nSect; i++){
        f1[i] = new TF1(Form("f1_phi%d", i+1), "[0]+[1]*x+[2]*x*x", phiMin[i], phiMax[i]);
        if(!tryF2e)
            f2[i] = new TF1(Form("f2_phi%d", i+1), "[0]+([1]+[2]*x+[3]*x*x)*exp(-x)", thMin, thMax);
        else
            f2e[i] = new TF1(Form("f2e_phi%d"), "[0]*x**[1]+[2]", thMin, thMax);
    }

    if(setInitParams & fexists("initFitParams.txt")){
        ifstream txtFile;
        TString aux;
        txtFile.open("initFitParams.txt");
        for(Int_t i = 0; i < nSect; i++){
            txtFile >> aux >> aux >> aux >> f1Params[i][0] >> f1Params[i][1] >> f1Params[i][2];
            txtFile >> aux >> aux >> aux >> f2Params[i][0] >> f2Params[i][1] >> f2Params[i][2] >> f2Params[i][3];
        }
        txtFile.close();
        for(Int_t i = 0; i < nSect; i++){
            for(Int_t j = 0; j < 4; j++){
                if(j != 3) f1[i]->SetParameter(j, f1Params[i][j]);
                f2[i]->SetParameter(j, f2Params[i][j]);
            }
        }
    }

    for(Int_t i = 0; i < nSect; i++){
        hF1r = (TH1F*) f->Get(Form("hF1_phi%d_m", i+1));
        hF2r = (TH1F*) f->Get(Form("hF2_phi%d_m", i+1));
        hF1r->Fit(Form("f1_phi%d", i+1), "REQ");
        f->cd();
        hF1r->Write(Form("%s_FIT", hF1r->GetName()));
        f1Params[i][0] = f1[i]->GetParameter(0);
        f1Params[i][1] = f1[i]->GetParameter(1);
        f1Params[i][2] = f1[i]->GetParameter(2);
        if(!tryF2e){
            hF2r->Fit(Form("f2_phi%d", i+1), "REQ");
            f->cd();
            hF2r->Write(Form("%s_FIT", hF2r->GetName()));
            f2Params[i][0] = f2[i]->GetParameter(0);
            f2Params[i][1] = f2[i]->GetParameter(1);
            f2Params[i][2] = f2[i]->GetParameter(2);
            f2Params[i][3] = f2[i]->GetParameter(3);
        }
        else{
            hF2r->Fit(Form("f2e_phi%d", i+1), "REQ");
            f->cd();
            hF2r->Write(Form("%s_FIT", hF2r->GetName()));
            f2eParams[i][0] = f2e[i]->GetParameter(0);
            f2eParams[i][1] = f2e[i]->GetParameter(1);
            f2eParams[i][2] = f2e[i]->GetParameter(2);
        }
    }

    for(Int_t i = 0; i < nSect; i++){
        f1[i]->Delete();
        if(!tryF2e)
            f2[i]->Delete();
        else
            f2e[i]->Delete();
    }
    hF1r->Delete();
    hF2r->Delete();

    f->Close();
    delete f;

    return;
}

void FitPCorr::writeParams(TString txtfName)
{
    ofstream txtFile;
    txtFile.open(txtfName);
    for(Int_t i = 0; i < nSect; i++){
        txtFile << "F1 Sector " << i+1 << " ";
        txtFile << f1Params[i][0] << "\t";
        txtFile << f1Params[i][1] << "\t";
        txtFile << f1Params[i][2] << "\t";
        txtFile << endl;
        txtFile << "F2 Sector " << i+1 << " ";
        txtFile << f2Params[i][0] << "\t";
        txtFile << f2Params[i][1] << "\t";
        txtFile << f2Params[i][2] << "\t";
        txtFile << f2Params[i][3] << "\t";
        txtFile << endl;
    }
    txtFile.close();
}

void FitPCorr::writeHistsBins(TString txtfName)
{
    TFile *f = new TFile(rootfName, "READ");
    TH1F *hF1;
    TH1F *hF2;
    Int_t phiBins = 60;
    Int_t thBins = 18;

    ofstream txtFile;
    txtFile.open(txtfName);
    for(Int_t i = 0; i < nSect; i++){
        hF1 = (TH1F*) f->Get(Form("hF1_phi%d_m_FIT", i+1));
        txtFile << "hF1_" << i+1 << endl;
        for(Int_t j = 0; j < phiBins; j++){
            txtFile << hF1->GetBinCenter(j) << "\t" <<  hF1->GetBinContent(j) << "\t" << hF1->GetBinError(j) << endl;
        }
        txtFile << endl;
        hF2 = (TH1F*) f->Get(Form("hF2_phi%d_m_FIT", i+1));
        txtFile << "hF2_" << i+1 << endl;
        for(Int_t j = 0; j < thBins; j++){
            txtFile << hF2->GetBinCenter(j) << "\t" << hF2->GetBinContent(j) << "\t" << hF2->GetBinError(j) << endl;
        }
        txtFile << endl;
    }
    txtFile.close();
}

void FitPCorr::printPlots()
{
    TFile *f = new TFile(rootfName, "READ");

    TH1F *hW = (TH1F*) f->Get("hW");
    TH1F *hWextra = (TH1F*) f->Get("hWextra");
    TH1F *hZ = (TH1F*) f->Get("hZ");

    TCanvas *c1 = new TCanvas("c1", "", 1600, 900);
    hW->Draw();
    c1->SaveAs("invMass.png");

    hWextra->Draw();
    hW->SetMarkerColor(kRed);
    hW->SetLineColor(kRed);
    hW->Draw("same");
    c1->SaveAs("invMassExtra.png");

    hZ->Draw();
    c1->SaveAs("zCoordinate.png");

    TH1F *hF1;
    TH1F *hF2;

    for(Int_t i = 0; i < nSect; i++){
        hF1 = (TH1F*) f->Get(Form("hF1_phi%d_m_FIT", i+1));
        hF1->Draw();
        hF1->GetYaxis()->SetRangeUser(0.95, 1.05);
        c1->SaveAs(Form("fitF1_phi%d.png", i));
        hF2 = (TH1F*) f->Get(Form("hF2_phi%d_m_FIT", i+1));
        hF2->Draw();
        hF2->GetYaxis()->SetRangeUser(0.95, 1.05);
        c1->SaveAs(Form("fitF2_phi%d.png", i));
    }

    hF1->Delete();
    hF2->Delete();

    delete c1;
    f->Close();
    delete f;
}

Bool_t FitPCorr::isInitElec()
{
    Bool_t result;
    result = fId->Status(0) > 0 && fId->Status(0) < 100 && fId->Charge(0) == -1 && fId->StatCC(0) > 0 && fId->StatSC(0) > 0;
    result = result && fId->StatDC(0) > 0 && fId->StatEC(0) > 0 && fId->DCStatus(0) > 0 && fId->SCStatus(0) == 33 && fId->Nphe(0) > 25;
    result = result && (fId->Etot(0) / 0.27 / 1.15 + 0.4) > fId->Momentum(0) && (fId->Etot(0) / 0.27 / 1.15 - 0.2 < fId->Momentum(0));
    result = result && (fId->Ein(0) + fId->Eout(0) > 0.8 * 0.27 * fId->Momentum(0)) && (fId->Ein(0) + fId->Eout(0) < 1.2 * 0.27 * fId->Momentum(0));
    result = result && fId->Eout(0) != 0 && fId->FidCheckCut() == 1;
    return result;
}

Bool_t FitPCorr::isPartProton(Int_t j)
{
    Bool_t result;
    result = j > 0 && fId->Charge(j) == 1 && fId->Status(j) > 0 && fId->Status(j) < 100 && fId->StatDC(j) > 0 && fId->DCStatus(j) > 0;
    result = result && fId->StatSC(j) > 0;
    if(fId->Momentum(j) > 1){
        result = result && fId->TimeCorr4(kMassP,j) >= -0.69 && fId->TimeCorr4(kMassP,j) <= 1.38;
    }
    else if(fId->Momentum(j) < 1){
        result = result && fId->TimeCorr4(kMassP,j) >= -3.78 && fId->TimeCorr4(kMassP,j) <= 6.75;
    }
    return result;
}

Float_t FitPCorr::ratioF1(Float_t p, Float_t theta)
{
    Float_t ratio;
    ratio = pCalc(theta)/p;
    return ratio;
}

Float_t FitPCorr::pCalc(Float_t theta){
    Float_t p;
    theta = TMath::Pi()*theta/180;
    p = eBeam/(1 + eBeam * (1 - TMath::Cos(theta))/kMassP);
    return p;
}


void FitPCorr::initHists(){
    hW = new TH1F("hW", "W no DIS", 100, 0.7, 1.2);
    hWextra = new TH1F("hWextra", "W no DIS", 100, 0.7, 1.2);
    hZ = new TH1F("hZ", "Z Coordinate", 100, -80, 20);

    TString sHF1Title;
    TString sHF2Title;
    TString sHF1MeanTitle;
    TString sHF2MeanTitle;
    TString sHF1eTitle;

    if(thCut < thMin) thMin = thCut;
    for(Int_t i = 0; i < nSect; i++){
        sHF1Title = Form("F1 vs Phi (Sector %d)", i+1);
        sHF2Title = Form("F2 vs Theta (Sector %d)", i+1);
        hF1[i] = new TH2F(Form("hF1_phi%d", i+1), sHF1Title, phiBins, phiMin[i], phiMax[i], f1Bins, f1Min, f1Max);
        hF2[i] = new TH2F(Form("hF2_phi%d", i+1), sHF2Title, thBins, thMin, thMax, f2Bins, f2Min, f2Max);
        hF1Mean[i] = new TH1F(Form("hF1_phi%d_m", i+1), sHF1MeanTitle, phiBins, phiMin[i], phiMax[i]);
        hF2Mean[i] = new TH1F(Form("hF2_phi%d_m", i+1), sHF2MeanTitle, thBins, thMin, thMax);
        for(Int_t j = 0; j < 4; j++){
            sHF1eTitle = Form("F1 vs Phi (Sector %d) %f < th < %f", i, thetaMin[j], thetaMax[j]);
            hF1Extra[i][j] = new TH2F(Form("hF1e_phi%d", i+1), sHF1eTitle, phiBins, phiMin[i], phiMax[i], thetaMax[j]-thetaMin[j], thetaMin[j], thetaMax[j]);
        }
    }
}

void FitPCorr::writeHists(TFile &f)
{
    f->cd();
    hW->Write();
    hZ->Write();
    hWextra->Write();
    for(Int_t i = 0; i < nSect; i++){
        hF1[i]->Write();
        hF2[i]->Write();
        hF1Mean[i]->Write();
        hF2Mean[i]->Write();
    }

    hW->Delete();
    hWextra->Delete();
    hZ->Delete();
    for(Int_t i = 0; i < nSect; i++){
        hF1[i]->Delete();
        hF2[i]->Delete();
        hF1Mean[i]->Delete();
        hF2Mean[i]->Delete();
    }
    f->Close();
    delete f;
}