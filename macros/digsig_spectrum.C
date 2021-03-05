//
// Plot the spectrum for each channel
// Used for CAEN readout at FTBF
//
#include <iostream>
#include <TH1.h>
#include <TFile.h>
#include <TString.h>

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

void digsig_spectrum( const char *rootfname = "dt5742.root" )
{
  gSystem->Load("libdigsig.so");

  TString savefname = "spectrum.root";
  cout << "Saving to " << savefname << endl;
  TFile *savefile = new TFile(savefname,"RECREATE");

  TString name;
  TH1 *h_adc[16] = {0};
  for (int ich=0; ich<NCH; ich++)
  {
    name = "h_adc"; name += ich;
    h_adc[ich] = new TH1F(name,name,450,-50,400);
  }

  TH1 *h_nhits = new TH1F("h_nhits","nhits",17,-0.5,16.5);
  TH2 *h2_nhits = new TH2F("h2_nhits","nhits",4,0,4,4,0,4);

  TCanvas *ac = new TCanvas("ac","ac",800,800);
  ac->Divide(4,4);
  DigAna digana(NCH,NSAMP);

  int nentries = digana.OpenRootFile(rootfname);
  cout << "Processing " << nentries << " events" << endl;
  Int_t nbadevents = 0;

  Double_t xatmin, ymin;
  for (int ievt=0; ievt<nentries; ievt++)
  {
    digana.ProcessEvent(ievt);

    Int_t nbadch = 0;
    Int_t nhitch = 0;   // num channels with a hit
    Double_t ampl[NCH]; // amplitudes

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *digsig = digana.GetSig(ich);
      digsig->CalcEventPed0(20.,60.);
      //cout << ievt << "\t" << digsig->GetPed0() << endl;
      digsig->LocMin(xatmin,ymin,280.,305.);
      //if ( ymin < -50. ) cout << ich << "\t" << xatmin << "\t" << ymin << endl;
      //
      ampl[ich] = -ymin;

      // found a hit
      if ( ymin<-60 )
      {
        int xpos = ich%4;
        int ypos = 3 - ich/4;

        h2_nhits->Fill(xpos,ypos);

        nhitch++;
      }

      // special cut for weird noisy events seen in ftbf march 2019 data
      if ( ampl[ich]>45 && ampl[ich]<60 )
      {
        nbadch++;
      }
    }

    h_nhits->Fill( nhitch );

    if ( nbadch<8 )
    {
      for (int ich=0; ich<NCH; ich++)
      {
        h_adc[ich]->Fill( -ymin );
      }
    }
    else
    {
      cout << "Found bad event " << ievt << endl;
      nbadevents++;
    }
  }

  for (int ich=0; ich<NCH; ich++)
  {
    ac->cd(ich+1);
    h_adc[ich]->Scale(1.0/nentries);
    h_adc[ich]->Draw();
    gPad->SetLogy(1);
  }

  TCanvas *bc = new TCanvas("c_position","nhits",800,800);
  h2_nhits->Draw("colz");
  bc->SaveAs(".png");

  // Number of hits
  TCanvas *cc = new TCanvas("c_nhits","nhits",800,800);
  h_nhits->SetLineColor(2);
  h_nhits->Draw();
  cc->SaveAs(".png");

  Double_t percent_empty = h_nhits->GetBinContent(1)/h_nhits->Integral(); // percentage of events with no hits
  cout << "Fraction of events with hits = " << 1.0 - percent_empty << endl;

  savefile->Write();
  //savefile->Close();
  //
  cout << "%bad " << (double)nbadevents/nentries << endl;
}

