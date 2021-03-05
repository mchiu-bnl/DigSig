#include <iostream>
#include <TCanvas.h>
#include <TFile.h>
#include <TString.h>
#include <TSystem.h>

#include <DigAna.h>

// Need to make symlinks to DigAna.h and DigSig.h
// Then compile with
//
// gSystem->Load("libdigsig.so");
// .L digsig_maketemplate.C+g
//

// sPHENIX Digitizers
//const int NCH = 64;
//const int NCH = 48;
const int NCH = 96;
//const int NCH = 2;
const int NSAMP = 20;
const Double_t MINGOODADC = 100;
const Double_t MAXGOODADC = 17000;
const Double_t MINFITRANGE = -10.;   // min x of fit
const Double_t MAXFITRANGE = 20.;  // max x of fit
const Double_t TIMEOFFSET = 0;
//const Double_t ped0[] = { 1584.3, 1497.5 };

/*
// DRS4
const int NCH = 4;
const int NSAMP = 1024;
const Double_t MINGOODADC = -0.1;   // min value of data to include in template fit
const Double_t MAXGOODADC = 0.52;   // max vlaue of data to include in template fit
const Double_t MINFITRANGE = 60.;   // min x of fit in data
const Double_t MAXFITRANGE = 150.;  // max x of fit
const Double_t TIMEOFFSET = 0;
*/

/*
// CAEN V1742
const int NCH = 16;
const int NSAMP = 1024;
const Double_t MINGOODADC = 40;     // min value of ped-subtracted data to include in template fit
const Double_t MAXGOODADC = 3000;   // max vlaue of data to include
const Double_t MINFITRANGE = 100.;  // min x of fit in data
const Double_t MAXFITRANGE = 170.;  // max x of fit
const Double_t TIMEOFFSET = 0;
*/

// CAEN V1742
/*
const int NCH = 16;
const int NSAMP = 20;
const Double_t MINGOODADC = -100;     // min value of ped-subtracted data to include in template fit
const Double_t MAXGOODADC = 13000;   // max vlaue of data to include
const Double_t MINFITRANGE = 0.;  // min x of fit in data
const Double_t MAXFITRANGE = 19.;  // max x of fit
const Double_t TIMEOFFSET = 0;
*/

const Int_t    TEMPLATE_NXBINS = 300;
const Int_t    TEMPLATE_NYBINS = 300;
const Int_t    TEMPLATE_MINX = -10;
const Int_t    TEMPLATE_MAXX = 20;

// need to figure out good begtime, endtime of template after fit
// need to figure out good number of points in histogram..
// List of skipped channels

// Skip all but certain ch's
void SelectChannels(DigAna *d)
{
  d->SkipAll();
  d->EnableCh(5);
  d->EnableCh(6);
  d->EnableCh(13);
  d->EnableCh(14);
}

void digsig_makesplinetemplate(const char *rootfname = "bb_2017_04_03_10_18.root")
{
  TFile *savefile = new TFile("spline_template.root","RECREATE");

  DigAna *digana = new DigAna(NCH,NSAMP);
  digana->SetTemplateMinMaxGoodADC(MINGOODADC, MAXGOODADC);
  digana->SetTemplateMinMaxFitRange(MINFITRANGE, MAXFITRANGE);
  digana->SetTimeOffset(TIMEOFFSET);

  //
  //digana->SetTemplateSize(120,12000,-5,10);
  //digana->SetTemplateSize(600,8000,-15,15);
  //digana->SetTemplateSize(600,8000,-35,35);
  digana->SetTemplateSize(TEMPLATE_NXBINS,TEMPLATE_NYBINS,TEMPLATE_MINX,TEMPLATE_MAXX);

  //SelectChannels(digana);

  // Read in pedestals
  TString pedfile(rootfname);
  pedfile.Prepend("PED/");
  pedfile.ReplaceAll(".root","_ped.txt");
  cout << "Ped file " << pedfile << endl;
  int npedfound = digana->SetPed0FromFile(pedfile);
  cout << "Found " << npedfound << " pedestals" << endl;

  int nentries = digana->OpenRootFile(rootfname);

  digana->FillSplineTemplate();
  digana->MakeAndWriteTemplate("sig_gen0");


  savefile->Write();
  savefile->Close();

  delete digana;
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

  //digana->SetTemplateSize(120,12000,-5,10);
  digana->SetTemplateSize(600,8000,-35,35);

  SelectChannels(digana);

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

  delete digana;
}

void digsig_maketemplate(const char *rootfname = "bb_2017_04_03_10_18.root")
{

  gSystem->Load("libdigsig.so");

  TCanvas *ac = new TCanvas("ac","ac",550,425);

  cout << "PASS 0" << endl;
  digsig_makesplinetemplate(rootfname);  // pass 0
  return;
  cout << "PASS 1" << endl;
  digsig_makefcntemplate(rootfname,1);  // pass 1
  cout << "PASS 2" << endl;
  digsig_makefcntemplate(rootfname,2);  // pass 2
 
}

