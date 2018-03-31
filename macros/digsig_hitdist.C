//
// Plot the number of hits in each channel
// Used for CAEN readout at FTBF
//
#include <iostream>
#include <TH1.h>

// BBC studies using New Digitizers
/*
const int NCH = 2;
const int NSAMP = 31;
const Int_t begin_sample = 0; // sample numbers for pedestal
const Int_t end_sample = 2;
*/

// DRS4 studies
/*
const int NCH = 4;
const int NSAMP = 1024;
const Double_t begin_sample = 20.0;  // sample ranges in ns for pedestal
const Double_t end_sample = 50.0;
*/

// CAEN DT5742
const int NCH = 16;
const int NSAMP = 1024;
const Double_t begin_sample = 30.0;  // sample ranges in ns for pedestal
const Double_t end_sample = 80.0;

void digsig_hitdist( const char *rootfname = "dt5742.root" )
{
  gSystem->Load("libdigsig.so");

  /*
  TString savefname = rootfname;
  savefname.ReplaceAll(".root","_ped.root");
  cout << savefname << endl;
  TFile *savefile = new TFile(savefname,"RECREATE");
  */

  TString name;
  TH1 *h_adc[16] = {0};
  for (int ich=0; ich<NCH; ich++)
  {
    name = "h_adc"; name += ich;
    h_adc[ich] = new TH1F(name,name,1101,-1000,100);
  }

  TH2 *h2_nhits = new TH2F("h2_nhits","nhits",4,0,4,4,0,4);

  TCanvas *ac = new TCanvas("ac","ac",800,800);
  ac->Divide(4,4);
  DigAna digana(NCH,NSAMP);

  int nentries = digana.OpenRootFile(rootfname);

  Double_t xatmin, ymin;
  for (int ievt=0; ievt<nentries; ievt++)
  {
    digana.ProcessEvent(ievt);

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *digsig = digana.GetSig(ich);
      digsig->CalcEventPed0(20.,60.);
      //cout << ievt << "\t" << digsig->GetPed0() << endl;
      digsig->LocMin(xatmin,ymin);
      //if ( ymin < -50. ) cout << ich << "\t" << xatmin << "\t" << ymin << endl;
      h_adc[ich]->Fill( ymin );

      if ( ymin<-60 )
      {
        int xpos = ich%4;
        int ypos = 3 - ich/4;

        h2_nhits->Fill(xpos,ypos);
      }
    }

  }

  for (int ich=0; ich<NCH; ich++)
  {
    ac->cd(ich+1);
    h_adc[ich]->Draw();
  }

  TCanvas *bc = new TCanvas("bc","nhits",800,800);
  h2_nhits->Draw("colz");
  bc->SaveAs("position.png");

}

