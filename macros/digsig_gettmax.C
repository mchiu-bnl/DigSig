//
// Get the max sample for each event in the time channels
//
//
#include <stdlib.h>
#include <TPad.h>
#include <TGraph.h>
#include "TGFrame.h"
#include "TGTab.h"
#include "TGButton.h"
#include "TRootEmbeddedCanvas.h"

#include "get_runstr.h"

TH1 *h_tmax[128]; // [pmt ch], max sample in event
TH2 *h2_tmax[2];  // [0 == time ch, 1 == chg ch], max sample in evt vs ch
TH2 *h2_wave[2];  // [0 == time ch, 1 == chg ch], all samples in evt vs ch
TH2 *h2_trange;   // adc at maxsamp vs ch

const int trigsamp = 3;


int digsig_gettmax(const char *fname = "dt5742.root", const int max_events = 0)
{
  TString name;
  TString title;
  TFile *tfile = new TFile(fname,"READ");

  Int_t evt;

  // MBD 1008
  const int NCH = 256;
  const int NSAMPLES = 31;
  const int NCANVAS = 4;

  //Double_t t[NCH][NSAMPLES];  // time
  //Double_t ch[NCH][NSAMPLES]; // voltage
  Float_t t[NCH][NSAMPLES];  // time
  Float_t ch[NCH][NSAMPLES]; // voltage

  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&evt);
  for (int ich=0; ich<NCH; ich++)
  {
      name = "t"; name += ich;
      tree->SetBranchAddress(name,&t[ich]);
      name = "ch"; name += ich;
      tree->SetBranchAddress(name,&ch[ich]);
  }

  TGraph *gpulse[NCH];
  for (int ich=0; ich<NCH; ich++)
  {
      gpulse[ich] = new TGraph();
      name = "gch"; name += ich;
      gpulse[ich]->SetName(name);
      gpulse[ich]->SetMarkerStyle(20);
      gpulse[ich]->SetMarkerSize(0.4);
      //gpulse[ich]->SetMarkerColor(ich+1);
      //gpulse[ich]->SetLineColor(ich+1);
      gpulse[ich]->SetMarkerColor(4);
      gpulse[ich]->SetLineColor(4);
  }

  // Book Histograms
  TString savefname = fname;
  savefname.ReplaceAll(".root","_tmax.root");
  TFile *savefile = new TFile(savefname,"RECREATE");

  for (int ich=0; ich<128; ich++)
  {
    name = "h_tmax"; name += ich;
    h_tmax[ich] = new TH1F(name,name,NSAMPLES,-0.5,NSAMPLES-0.5);
    h_tmax[ich]->SetXTitle("sample");
    h_tmax[ich]->SetYTitle("ch");
  }
  h2_tmax[0] = new TH2F("h2_ttmax","time tmax vs ch",NSAMPLES,-0.5,NSAMPLES-0.5,128,0,128);
  h2_tmax[1] = new TH2F("h2_qtmax","chg tmax vs ch",NSAMPLES,-0.5,NSAMPLES-0.5,128,0,128);
  h2_wave[0] = new TH2F("h2_twave","time adc vs ch",NSAMPLES,-0.5,NSAMPLES-0.5,128,0,128);
  h2_wave[1] = new TH2F("h2_qwave","chg adc vs ch",NSAMPLES,-0.5,NSAMPLES-0.5,128,0,128);

  h2_trange = new TH2F("h2_trange","tadc at trig samp vs ch",1600,0,16384,128,0,128);

  for (int itype=0; itype<2; itype++)
  {
    h2_tmax[itype]->SetXTitle("sample");
    h2_tmax[itype]->SetYTitle("ch");

    h2_wave[itype]->SetXTitle("sample");
    h2_wave[itype]->SetYTitle("ch");
  }
 
  int nskipped = 0; // number of skipped, uninteresting events
  int plotevt = -1; // an event that was specified to plot
  int interactive = 1;  // whether to have interactive plotting

  Long64_t nentries = tree->GetEntries();
  cout << "Found " << nentries << " events" << endl;
  if ( max_events > 0 )
  {
    nentries = max_events;
    cout << "Processing only " << max_events << endl;
  }
  int evt_processed = 0;
  for (int ievt=0; ievt<nentries; ievt++)
  {
    //if ( ievt<1078 ) continue;
    tree->GetEntry(ievt);
    //cout << "evt " << evt << endl;

    // Read in data
    for (int ich=0; ich<NCH; ich++)
    {
      int tq = (ich/8)%2;   // 0=time ch, 1=charge ch
      int pmtch = (ich/16)*8 + ich%8;

      float ymax = -9999.;
      float maxsamp = -9999.;
      for (int isamp=0; isamp<NSAMPLES; isamp++) 
      {
        gpulse[ich]->SetPoint(isamp,t[ich][isamp],ch[ich][isamp]);
        h2_wave[tq]->Fill( isamp , pmtch, ch[ich][isamp] );

        if ( ch[ich][isamp] > ymax )
        {
          ymax = ch[ich][isamp];
          maxsamp = t[ich][isamp];
        }
      }

      h_tmax[pmtch]->Fill( maxsamp );
      h2_tmax[tq]->Fill( maxsamp, pmtch );

      h2_trange->Fill( ch[ich][trigsamp] , pmtch );
    }

    evt_processed++;
  }

  h2_wave[0]->Scale(1.0/evt_processed);
  h2_wave[1]->Scale(1.0/evt_processed);


  TString dir = "results/";
  dir += get_runstr(fname);
  dir += "/";
  name = "mkdir -p " + dir;
  gSystem->Exec( name );
  TCanvas *ac[100];

  //tmax
  ac[0] = new TCanvas("c_ttmax","time tmax",425*1.5,550*1.5);
  h2_tmax[0]->Draw("colz");
  name = dir + "t_tmax.png";
  cout << name << endl;
  ac[0]->Print( name );
  ac[1] = new TCanvas("c_qtmax","chg tmax",425*1.5,550*1.5);
  h2_tmax[1]->Draw("colz");
  name = dir + "q_tmax.png";
  cout << name << endl;
  ac[1]->Print( name );

  //wave
  ac[2] = new TCanvas("c_twave","time wave",425*1.5,550*1.5);
  h2_wave[0]->Draw("colz");
  name = dir + "t_wave.png";
  cout << name << endl;
  ac[2]->Print( name );
  ac[3] = new TCanvas("c_qwave","chg wave",425*1.5,550*1.5);
  h2_wave[1]->Draw("colz");
  name = dir + "q_wave.png";
  cout << name << endl;
  ac[3]->Print( name );

  //ttrange
  ac[4] = new TCanvas("c_trange","trange",425*1.5,550*1.5);
  h2_trange->Draw("colz");
  name = dir + "trange.png";
  cout << name << endl;
  ac[4]->Print( name );

  savefile->Write();

  return 1;
}

