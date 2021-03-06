#include <iostream>

// DRS4 studies
/*
const int NCH = 4;
const int NSAMP = 1024;
const Double_t begin_sample = 20.0;  // sample ranges in ns for pedestal
const Double_t end_sample = 50.0;
*/

// BBC PMT with CAEN DT5742
/*
const int NCH = 16;
const int NSAMP = 1024;
const Double_t begin_sample = 30.0;  // sample ranges in ns for pedestal
const Double_t end_sample = 80.0;
*/

// BBC studies using New Digitizers
/*
const int NCH = 2;
const int NSAMP = 31;
const Int_t begin_sample = 0; // sample numbers for pedestal
const Int_t end_sample = 2;
*/

// MBD D/S Testing
const int MAXCH = 256;
const int NSAMP = 20;
const Int_t begin_sample = 0;  // sample ranges in ns for pedestal
const Int_t end_sample = 1;

// NCH total number of channels (including charge channels)
void digsig_calcped( const char *rootfname = "dt5742.root", int NCH=MAXCH )
{
  gSystem->Load("libdigsig.so");

  TString savefname = rootfname;
  savefname.ReplaceAll(".root","_ped.root");
  cout << savefname << endl;
  TFile *savefile = new TFile(savefname,"RECREATE");

  ifstream configfile("digsig.cfg");
  if ( configfile.is_open() )
  {
    string junk;
    configfile >> junk >> NCH;

    cout << "Found config file digsig.cfg" << endl;
    cout << "Setting NCH = " << NCH << endl;
  }

  TCanvas *ac = new TCanvas("ac","ac",550,425);
  DigAna digana(NCH,NSAMP);

  int nentries = digana.OpenRootFile(rootfname);
  cout << "nevents = " << nentries << endl;
  //nentries = 10;

  for (int ievt=0; ievt<nentries; ievt++)
  {
    digana.ProcessEvent(ievt);
    digana.FillPed0(begin_sample,end_sample);
  }

  TString pedfname = rootfname;
  gSystem->Exec("mkdir -p PED");
  pedfname.ReplaceAll(".root","_ped.txt");
  pedfname.Prepend("PED/");
  cout << pedfname << endl;
  ofstream pedfile(pedfname.Data());

  std::vector<Double_t> ped(NCH);
  std::vector<Double_t> pedrms(NCH);
  for (int ich=0; ich<NCH; ich++)
  {
    DigSig *digsig = digana.GetSig(ich);
    ped[ich] = digsig->GetPed0();
    pedrms[ich] = digsig->GetPed0RMS();
    cout << ich << "\t" << ped[ich] << "\t" << pedrms[ich] << endl;
    pedfile << ich << "\t" << ped[ich] << "\t" << pedrms[ich] << endl;
  }
  pedfile.close();
  savefile->Write();
}

