#include <iostream>
#include <TCanvas.h>
#include <TFile.h>
#include <TString.h>
#include <TSystem.h>

#include <DigAna.h>
// have to compile using 
// gSystem->Load("libdigsig.so");
// .L digsig_maketemplate.C+g

/* 
 // sPHENIX Digitizers
const int NCH = 2;
const int NSAMP = 31;
const Double_t MINGOODADC = 100;
const Double_t MAXGOODADC = 17000;
const Double_t MINFITRANGE = 0.;   // min x of fit
const Double_t MAXFITRANGE = 31.;  // max x of fit
const Double_t TIMEOFFSET = 0;
//const Double_t ped0[] = { 1584.3, 1497.5 };
*/

 // DRS4
const int NCH = 4;
const int NSAMP = 1024;
const Double_t MINGOODADC = -0.1;   // min value of data to include
const Double_t MAXGOODADC = 0.52;   // max vlaue of data to include
const Double_t MINFITRANGE = 60.;   // min x of fit in data
const Double_t MAXFITRANGE = 150.;  // max x of fit
const Double_t TIMEOFFSET = 0;

void digsig_makesplinetemplate(const char *rootfname = "bb_2017_04_03_10_18.root")
{
  TFile *savefile = new TFile("spline_template.root","RECREATE");

  DigAna digana(NCH,NSAMP);
  digana.SetTemplateMinMaxGoodADC(MINGOODADC, MAXGOODADC);
  digana.SetTemplateMinMaxFitRange(MINFITRANGE, MAXFITRANGE);
  digana.SetTimeOffset(TIMEOFFSET);
  digana.SetTemplateSize(120,12000,-5,10);

  // Read in pedestals
  TString pedfile(rootfname);
  pedfile.Prepend("PED/");
  pedfile.ReplaceAll(".root","_ped.txt");
  cout << "Ped file " << pedfile << endl;
  digana.SetPed0FromFile(pedfile);

  int nentries = digana.OpenRootFile(rootfname);

  digana.FillSplineTemplate();
  digana.MakeAndWriteTemplate("sig_gen0");

  savefile->Write();
  savefile->Close();
}

// always starts at pass 1
void digsig_makefcntemplate(const char *rootfname = "bb_2017_04_03_10_18.root", const int pass = 1)
{
  TString name = "fcn_template"; name += pass; name += ".root";
  TFile *savefile = new TFile(name,"RECREATE");

  DigAna *digana = new DigAna(NCH,NSAMP);
  digana->SetTemplateMinMaxGoodADC(MINGOODADC, MAXGOODADC);
  digana->SetTemplateMinMaxFitRange(MINFITRANGE, MAXFITRANGE);
  digana->SetTimeOffset(TIMEOFFSET);
  digana->SetTemplateSize(120,12000,-5,10);

  // Read in pedestals
  TString pedfile(rootfname);
  pedfile.Prepend("PED/");
  pedfile.ReplaceAll(".root","_ped.txt");
  cout << "Ped file " << pedfile << endl;
  digana->SetPed0FromFile(pedfile);

  name = "sig_gen"; name += (pass-1);
  digana->ReadTemplate(name);

  int nentries = digana->OpenRootFile(rootfname);

  digana->FillFcnTemplate();
  cout << "Done Filling Templates" << endl;

  name = "sig_gen"; name += pass;
  digana->MakeAndWriteTemplate(name);

  savefile->Write();
  savefile->Close();
}

void digsig_maketemplate(const char *rootfname = "bb_2017_04_03_10_18.root")
{
  gSystem->Load("libdigsig.so");

  TCanvas *ac = new TCanvas("ac","ac",550,425);

  digsig_makesplinetemplate(rootfname);  // pass 0
  digsig_makefcntemplate(rootfname,1);  // pass 1
  digsig_makefcntemplate(rootfname,2);  // pass 2
 
}

