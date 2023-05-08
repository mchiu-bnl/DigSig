//
// Plot the spectrum for each channel
// Used for CAEN readout at FTBF
//
#include <iostream>
#include <TH1.h>
#include <TFile.h>
#include <TString.h>
#include <DigSig.h>
#include <DigAna.h>

// BBC studies using New Digitizers
/*
const int NCH = 2;
const int NSAMP = 31;
const Int_t begin_ped = 0; // sample numbers for pedestal
const Int_t end_ped = 2;
*/

// DRS4 studies
const int NCH = 4;
const int NSAMP = 1024;
const Double_t begin_ped = 40.0;  // sample ranges in ns for pedestal
const Double_t end_ped = 50.0;

// CAEN DT5742
/*
const int NCH = 16;
const int NSAMP = 1024;
const Double_t begin_ped = 30.0;  // sample ranges in ns for pedestal
const Double_t end_ped = 80.0;
*/

const int MAXCH = 16; // max number of channels to process

R__LOAD_LIBRARY(libdigsig.so)

void digsig_spectrum( const char *rootfname = "dt5742.root" )
{
  gSystem->Load("libdigsig.so");

  TString savefname = "spectrum.root";
  cout << "Saving to " << savefname << endl;
  TFile *savefile = new TFile(savefname,"RECREATE");

  TString name;
  TH1 *h_adc[MAXCH] = {0};
  for (int ich=0; ich<NCH; ich++)
  {
    name = "h_adc"; name += ich;
    //h_adc[ich] = new TH1F(name,name,450,-50,400);
    h_adc[ich] = new TH1F(name,name,40,-0.01,0.2);
  }

  TH1 *h_nhits = new TH1F("h_nhits","nhits",17,-0.5,16.5);
  TH2 *h2_nhits = new TH2F("h2_nhits","nhits",4,0,4,4,0,4);

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
      digsig->CalcEventPed0(begin_ped,end_ped);
      //cout << ievt << "\t" << ich << "\t" << digsig->GetPed0() << endl;

      digsig->LocMin(xatmin,ymin,90.,100.);
      //if ( ymin < 0. ) cout << ievt << "\t" << ich << "\t" << xatmin << "\t" << ymin << endl;
 
      ampl[ich] = -ymin;

      for (int ich=0; ich<NCH; ich++)
      {
        h_adc[ich]->Fill( ampl[ich] );
      }

      // found a hit
      if ( ymin<-60 )
      {
        int xpos = ich%4;
        int ypos = 3 - ich/4;

        h2_nhits->Fill(xpos,ypos);

        nhitch++;
      }

      // special cut for weird noisy events seen in ftbf march 2019 data
      /*
      if ( ampl[ich]>45 && ampl[ich]<60 )
      {
        nbadch++;
      }
      */
    }

    h_nhits->Fill( nhitch );

    /*
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
    */
  }

  TCanvas *ac = new TCanvas("ac","ac",1200,400);
  ac->Divide(4,1);
  for (int ich=0; ich<NCH; ich++)
  {
    ac->cd(ich+1);
    //h_adc[ich]->Scale(1.0/nentries);
    h_adc[ich]->Draw();
    //gPad->SetLogy(1);

    double mean = h_adc[ich]->GetMean();
    double meanerr = h_adc[ich]->GetMeanError();
    cout << ich << "\t" << mean << "\t" << meanerr << endl;
  }

  /*
  TCanvas *bc = new TCanvas("c_position","nhits",800,800);
  h2_nhits->Draw("colz");
  bc->SaveAs(".png");

  // Number of hits
  TCanvas *cc = new TCanvas("c_nhits","nhits",800,800);
  h_nhits->SetLineColor(2);
  h_nhits->Draw();
  cc->SaveAs(".png");
  */

  Double_t percent_empty = h_nhits->GetBinContent(1)/h_nhits->Integral(); // percentage of events with no hits
  cout << "Fraction of events with hits = " << 1.0 - percent_empty << endl;

  savefile->Write();
  //savefile->Close();
  //
  cout << "%bad " << (double)nbadevents/nentries << endl;
}

