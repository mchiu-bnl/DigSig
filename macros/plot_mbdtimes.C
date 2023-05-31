//
// Plots the charge and time distributions from the *_times.root file
//
#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TTree.h>
#include <TFile.h>
#include <TGraphErrors.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "get_runstr.h"

const int MAXRUNS = 11;
const int MAXCH = 256;
const int MAXBOARDS = MAXCH/16;     // FEM boards
int NUMPMT = 128;        // number of PMTs
int NCH = MAXCH;                 // total number of channels (including charge channels)
int NBOARDS;

TH1 *h_ampl[MAXRUNS][MAXCH];    //[run][ch] 
TF1 *fgaus[MAXRUNS][MAXCH];
int nrun = 0;
int run_number[MAXRUNS];
float f_peak[MAXCH][MAXRUNS];
float f_peakerr[MAXCH][MAXRUNS];
TGraphErrors *g_laserscan[MAXCH];

TH1 *h_qsum[2]; // [arm]
TH2 *h2_tt[MAXRUNS];     // time in t-ch
TH2 *h2_tq[MAXRUNS];     // time in q-ch
TH2 *h2_q[MAXRUNS];      // charge

const int verbose = 0;

TString name;
TString title;

using namespace std;

// 
void anafile(const char *tfname = "prdf_478_times.root", const int nrun = 0)
{
  cout << "tfname " << tfname << endl;

  // Book Histograms, etc
  for (int ich=0; ich<NCH; ich++)
  {
    name = "h_bbc"; name += ich; name += "_"; name += nrun;
    title = name;
    h_ampl[nrun][ich] = new TH1F(name,title,1610,-100,16000);
  }

  for (int iarm=0; iarm<2; iarm++)
  {
    name = "h_qsum"; name += iarm;
    title = name;
    h_qsum[iarm] = new TH1F(name, title,1600,0,16000);
  }

  name = "h2_tt"; name += nrun;
  h2_tt[nrun] = new TH2F(name,name,300,-15.,15.,NUMPMT,-0.5,NUMPMT-0.5);
  name = "h2_tq"; name += nrun;
  h2_tq[nrun] = new TH2F(name,name,300,-15.,15.,NUMPMT,-0.5,NUMPMT-0.5);
  name = "h2_q"; name += nrun;
  h2_q[nrun] = new TH2F(name,name,4096,-100.,16384.,NUMPMT,-0.5,NUMPMT-0.5);

  Int_t   f_evt;
  Float_t f_tt[NUMPMT];  // time from t-channels
  Float_t f_tq[NUMPMT];  // time from q-channels
  Float_t f_q[NUMPMT];  // voltage

  cout << "tfname " << tfname << endl;

  TFile *tfile = new TFile(tfname,"READ");
  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&f_evt);
  for (int ipmt=0; ipmt<NUMPMT; ipmt++)
  {
    name = "tt"; name += ipmt;
    tree->SetBranchAddress(name,&f_tt[ipmt]);
    name = "tq"; name += ipmt;
    tree->SetBranchAddress(name,&f_tq[ipmt]);
    name = "q"; name += ipmt;
    tree->SetBranchAddress(name,&f_q[ipmt]);
  }

  int nentries = tree->GetEntries();
  for (int ientry=0; ientry<nentries; ientry++)
  {
    tree->GetEntry(ientry);

    for (int ich=0; ich<NCH; ich++)
    {
      int sn = ich/128;     // south or north
      int quad = ich/64;    // quadrant
      int pmtch = (ich/16)*8 + ich%8;
      int tq = (ich/8)%2;   // 0 = T-channel, 1 = Q-channel

      //cout << evt << "\t" << t[1] << "\t" << ch[1] << "\t" << t[14] << "\t" << ch[14] << endl;
      h2_tt[nrun]->Fill( f_tt[ich], pmtch );
      h2_tq[nrun]->Fill( f_tq[ich], pmtch );
      h2_q[nrun]->Fill( f_q[ich], pmtch );
      
      if ( tq == 1 )    // charge channel
      {
        h_ampl[nrun][ich]->Fill( f_q[ich] );
        if ( f_tt[ich]>8 && f_tt[ich]<16 ) h_qsum[sn]->Fill( f_q[ich] );
      }
      else
      {
        if ( f_q[ich]>200 ) h_ampl[nrun][ich]->Fill( f_q[ich] );
      }
    }
  }

  // Fit gaussians to get the average amplitude
  /*
  for (int ich=0; ich<NCH; ich++)
  {
    //cout << "Fitting ch " << ich << endl;
    name = "fgaus"; name += ich; name += "_"; name += nrun;
    fgaus[nrun][ich] = new TF1(name,"gaus",-100,16000);
    fgaus[nrun][ich]->SetLineColor(4);
    fgaus[nrun][ich]->SetParameters(1000,10000,10);
    h_ampl[nrun][ich]->Fit( fgaus[nrun][ich], "NQR" );

    f_peak[ich][nrun] = fgaus[nrun][ich]->GetParameter(1);
    f_peakerr[ich][nrun] = fgaus[nrun][ich]->GetParError(1);
  }
  */

  // Draw time and charge distributions
  const int NCANVAS = 4;
  TCanvas *c_charge[NCANVAS];
  TCanvas *c_time[NCANVAS];

  if ( verbose )
  {
    for (int icv = 0; icv<NCANVAS; icv++)
    {
      name = "c_charge"; name += icv;
      c_charge[icv] = new TCanvas(name,name,1600,800);
      c_charge[icv]->Divide(8,4);

      name = "c_time"; name += icv;
      c_time[icv] = new TCanvas(name,name,1600,800);
      c_time[icv]->Divide(8,4);
    }
  }

  for (int ich=0; ich<NCH; ich++)
  {
    int sn = ich/128;     // south or north
    int quad = ich/64;    // quadrant
    int pmtch = (ich/16)*8 + ich%8;  // pmtch
    int tq = (ich/8)%2;   // 0 = T-channel, 1 = Q-channel

    if ( verbose )
    {
      if ( tq==1 )  // charge channel
      {
        c_charge[quad]->cd( pmtch%32 + 1 );
        h_ampl[nrun][ich]->Draw();
        h_ampl[nrun][ich]->Rebin(10);
        gPad->SetLogy(1);
      }
      else          // time channel
      {
        c_time[quad]->cd( pmtch%32 + 1 );
        h_ampl[nrun][ich]->Draw();
        h_ampl[nrun][ich]->Rebin(10);
        //gPad->SetLogy(1);
      }
    }
  }

  TString dir = "results/";
  dir += get_runstr(tfname);
  dir += "/";
  name = "mkdir -p " + dir;
  gSystem->Exec( name );
  //gSystem->MakeDirectory( dir );
  name = "cd " + dir;
  gSystem->Exec( name );
  //gSystem->ChangeDirectory( dir );

  // Save the canvases
  if ( verbose )
  {
    for (int icv = 0; icv<NCANVAS; icv++)
    {
      c_charge[icv]->SaveAs( ".png" );
      c_time[icv]->SaveAs( ".png" );
    }
  }

  TCanvas *ac[100];
  int icvs = 0;
  ac[icvs] = new TCanvas("c_tt","time in t-ch",800,400);
  h2_tt[nrun]->Draw("colz");
  name = dir + "h2_tt.png";
  ac[icvs]->SaveAs( name );
  icvs++;
  ac[icvs] = new TCanvas("c_tq","time in q-ch",800,400);
  h2_tq[nrun]->Draw("colz");
  name = dir + "h2_tq.png";
  ac[icvs]->SaveAs( name );
  icvs++;
  ac[icvs] = new TCanvas("c_q","charge in pmt",800,400);
  h2_q[nrun]->Draw("colz");
  name = dir + "h2_q.png";
  ac[icvs]->SaveAs( name );
  icvs++;
  /*
  ac[icvs] = new TCanvas("ac","ac",800,400);
  h_qsum[0]->Draw();
  ac[icvs]->SaveAs( ".png" );
  icvs++;
  ac[icvs] = new TCanvas("c_qsum","bc",800,400);
  h_qsum[1]->Draw();
  bc->SaveAs( ".png" );
  */

}

void plot_mbdtimes(const char *fname = "prdf.root")
{
    NCH = MAXCH;                 // total number of channels (including charge channels)
    NBOARDS = MAXBOARDS;

    // Get the number of actual channels to process
    ifstream configfile("digsig.cfg");
    if ( configfile.is_open() )
    {
        string junk;
        configfile >> junk >> NCH;
        NBOARDS = NCH/16;

        cout << "Found config file digsig.cfg" << endl;
        cout << "Setting NCH = " << NCH << endl;
    }

    TString rootfname;

    // get name of root file
    rootfname = fname;
    cout << "Processing " << rootfname << endl;

    anafile( rootfname, 0 );
}
