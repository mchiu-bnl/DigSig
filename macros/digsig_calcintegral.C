#include <iostream>
#include <TSpectrum.h>

// BBC studies using New Digitizers
/*
const int NCH = 2;
const int NSAMP = 31;
const Int_t begin_sample = 0; // sample numbers for pedestal
const Int_t end_sample = 2;
*/

// DRS4 studies
const int NCH = 4;
const int NSAMP = 1024;
const Double_t ped_begin_time = 20.0;  // sample ranges in ns for pedestal
const Double_t ped_end_time = 80.0;
const Double_t integ_begin_time = -5;  // ranges around peak for integration of signal (ns)
const Double_t integ_end_time = +13.;

// BBC PMT with CAEN DT5742
/*
const int NCH = 16;
const int NSAMP = 1024;
const Double_t begin_sample = 30.0;  // sample ranges in ns for pedestal
const Double_t end_sample = 100.0;
*/

void digsig_calcintegral( const char *rootfname = "digsig.root" )
{
  gSystem->Load("libdigsig.so");

  TString savefname = rootfname;
  savefname.ReplaceAll(".root","_integ.root");
  cout << savefname << endl;
  TFile *savefile = new TFile(savefname,"RECREATE");
  TH1 *h_integral[NCH];
  TString name;
  for (int ich=0; ich<NCH; ich++)
  {
    name = "h_integral"; name += ich;
    h_integral[ich] = new TH1F(name,name,1000,1,0); // auto binned
  }

  TCanvas *ac = new TCanvas("ac","ac",550,425);
  DigAna digana(NCH,NSAMP);

  int nentries = digana.OpenRootFile(rootfname);

  // Invert should be done when creating original rootfile!!!!!
  // Default should be positive pulses
  //digana.Invert();  

  // Set Calibrations
  /*
  // Use globally determined pedestal
  TString pedfname = rootfname;
  pedfname.ReplaceAll(".root","_ped.txt");
  Int_t index = pedfname.Last('/');
  if ( index>=0 ) pedfname.Remove(0,index+1);
  pedfname.Prepend("PED/");
  int pedstatus = digana.SetPed0FromFile( pedfname );
  if ( pedstatus<0 )
  {
    cerr << "ERROR: can't open file " << pedfname << endl;
    return 0;
  }
  */

  // Use Event-by-Event Pedestal
  digana.SetEventPed0Range(ped_begin_time,ped_end_time);

  for (int ievt=0; ievt<nentries; ievt++)
  {
    digana.ProcessEvent(ievt);
    digana.CalcIntegralAroundPeak(integ_begin_time,integ_end_time);

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *sig = digana.GetSig(ich);
      Double_t integ = sig->GetIntegral();
      Double_t xp, yp;
      sig->LocMax(xp,yp);
      if ( xp > 140. ) continue;
      h_integral[ich]->Fill( integ );
      if ( integ < -5. ) 
      {
        cout << ich << "\t" << ievt << "\t" << integ << "\t" << xp << endl;
      }
    }
  }

  // Save data to output text file
  TString integfname = rootfname;
  integfname.ReplaceAll(".root","_integ.txt");
  cout << integfname << endl;
  ofstream integoutfile(integfname.Data());

  for (int ich=0; ich<NCH; ich++)
  {
    integoutfile << ich << "\t" << h_integral[ich]->GetMean() << "\t" << h_integral[ich]->GetRMS() << endl;
  }
  integoutfile.close();

  savefile->Write();
}

