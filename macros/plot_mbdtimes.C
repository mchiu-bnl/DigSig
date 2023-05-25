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
int NCH;                 // total number of channels (including charge channels)
int NBOARDS;

TH1 *h_laseramp[MAXRUNS][MAXCH];    //[run][ch] 
TF1 *fgaus[MAXRUNS][MAXCH];
int nrun = 0;
int run_number[MAXRUNS];
float hvscale[MAXRUNS];
float laseramp[MAXCH][MAXRUNS];
float laseramperr[MAXCH][MAXRUNS];
TGraphErrors *g_laserscan[MAXCH];

TH1 *h_qsum[2]; // [arm]

const int verbose = 1;

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
    h_laseramp[nrun][ich] = new TH1F(name,title,1610,-100,16000);
  }

  for (int iarm=0; iarm<2; iarm++)
  {
    name = "h_qsum"; name += iarm;
    title = name;
    h_qsum[iarm] = new TH1F(name, title,1600,0,16000);
  }

  Int_t   evt;
  Float_t tt[NUMPMT];  // time from t-channels
  Float_t tq[NUMPMT];  // time from q-channels
  Float_t ch[NUMPMT];  // voltage

  cout << "tfname " << tfname << endl;

  TFile *tfile = new TFile(tfname,"READ");
  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&evt);
  for (int ich=0; ich<NCH; ich++)
  {
    name = "tt"; name += ich;
    tree->SetBranchAddress(name,&t[ich]);
    name = "q"; name += ich;
    tree->SetBranchAddress(name,&ch[ich]);
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
      if ( tq == 1 )    // charge channel
      {
        h_laseramp[nrun][ich]->Fill( ch[ich] );
        if ( t[ich]>8&&t[ich]<16 ) h_qsum[sn]->Fill( ch[ich] );
      }
      else
      {
        if ( ch[ich]>200 ) h_laseramp[nrun][ich]->Fill( ch[ich] );
      }
    }
  }

  // Fit gaussians to get the average amplitude
  for (int ich=0; ich<NCH; ich++)
  {
    //cout << "Fitting ch " << ich << endl;
    name = "fgaus"; name += ich; name += "_"; name += nrun;
    fgaus[nrun][ich] = new TF1(name,"gaus",-100,16000);
    fgaus[nrun][ich]->SetLineColor(4);
    fgaus[nrun][ich]->SetParameters(1000,10000,10);
    h_laseramp[nrun][ich]->Fit( fgaus[nrun][ich], "NQR" );

    float ampl = fgaus[nrun][ich]->GetParameter(1);
    float amplerr = fgaus[nrun][ich]->GetParError(1);
    laseramp[ich][nrun] = ampl;
    laseramperr[ich][nrun] = amplerr;
  }

  // Draw time and charge distributions
  const int NCANVAS = 4;
  TCanvas *c_charge[NCANVAS];
  TCanvas *c_time[NCANVAS];
  for (int icv = 0; icv<NCANVAS; icv++)
  {
    name = "c_charge"; name += icv;
    c_charge[icv] = new TCanvas(name,name,1600,800);
    c_charge[icv]->Divide(8,4);

    name = "c_time"; name += icv;
    c_time[icv] = new TCanvas(name,name,1600,800);
    c_time[icv]->Divide(8,4);
  }

  for (int ich=0; ich<NCH; ich++)
  {
    int sn = ich/128;     // south or north
    int quad = ich/64;    // quadrant
    int pmtch = (ich/16)*8 + ich%8;  // pmtch
    int tq = (ich/8)%2;   // 0 = T-channel, 1 = Q-channel
    if ( tq==1 )  // charge channel
    {
      c_charge[quad]->cd( pmtch%32 + 1 );
      h_laseramp[nrun][ich]->Draw();
      h_laseramp[nrun][ich]->Rebin(10);
      gPad->SetLogy(1);
    }
    else          // time channel
    {
      c_time[quad]->cd( pmtch%32 + 1 );
      h_laseramp[nrun][ich]->Draw();
      h_laseramp[nrun][ich]->Rebin(10);
      //gPad->SetLogy(1);
    }
  }

  TString dir = "results/";
  dir += get_runstr(fname);
  dir += "/";
  //name = "mkdir -p " + dir;
  gSystem->MakeDirectory( dir );
  //name = "cd " + dir;
  gSystem->ChangeDirectory( dir );

  // Save the canvases
  for (int icv = 0; icv<NCANVAS; icv++)
  {
    c_charge[icv]->SaveAs( ".png" );
    c_time[icv]->SaveAs( ".png" );
  }

  TCanvas *ac = new TCanvas("ac","ac",800,400);
  h_qsum[0]->Draw();
  ac->SaveAs( ".png" )
  TCanvas *bc = new TCanvas("bc","bc",800,400);
  h_qsum[1]->Draw();
  bc->SaveAs( ".png" )

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

    anafile( rootfname );
}
