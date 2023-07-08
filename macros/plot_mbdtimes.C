//
// Plots the charge and time distributions from the *_mbd.root (or *_times.root) file
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
#include "BbcGeom.h"

const int MAXRUNS = 11;
const int MAXCH = 256;
const int MAXBOARDS = MAXCH/16;     // FEM boards
const int NUM_PMT = 128;                   // number of PMTs
int NCH = MAXCH;                    // total number of channels (including charge channels)
int NBOARDS;

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
TH1 *h_q[MAXRUNS][NUM_PMT];    //[run][ipmt] 

const int verbose = 0;

TString name;
TString title;

using namespace std;

// 
void anafile(const char *tfname = "prdf_478_times.root", const int nrun = 0)
{
  cout << "tfname " << tfname << endl;

  // Book Histograms, etc
  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
  {
    name = "h_bbcq"; name += ipmt; name += "_"; name += nrun;
    title = name;
    h_q[nrun][ipmt] = new TH1F(name,title,1610,-100,16000);
  }

  for (int iarm=0; iarm<2; iarm++)
  {
    name = "h_qsum"; name += iarm;
    title = name;
    h_qsum[iarm] = new TH1F(name, title,1600,0,16000);
  }

  name = "h2_tt"; name += nrun;
  h2_tt[nrun] = new TH2F(name,name,300,-15.,15.,NUM_PMT,-0.5,NUM_PMT-0.5);
  name = "h2_tq"; name += nrun;
  h2_tq[nrun] = new TH2F(name,name,300,-15.,15.,NUM_PMT,-0.5,NUM_PMT-0.5);
  name = "h2_q"; name += nrun;
  h2_q[nrun] = new TH2F(name,name,4096,-100.,16384.,NUM_PMT,-0.5,NUM_PMT-0.5);

  Int_t   f_evt;
  Float_t f_tt[NUM_PMT];  // time from t-channels
  Float_t f_tq[NUM_PMT];  // time from q-channels
  Float_t f_q[NUM_PMT];  // voltage

  cout << "tfname " << tfname << endl;

  TFile *tfile = new TFile(tfname,"READ");
  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&f_evt);
  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
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

    for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
    {
      int sn = ipmt/64;     // south or north
      int quad = ipmt/32;    // quadrant

      //cout << evt << "\t" << t[1] << "\t" << ch[1] << "\t" << t[14] << "\t" << ch[14] << endl;
      h2_tt[nrun]->Fill( f_tt[ipmt], ipmt );
      h2_tq[nrun]->Fill( f_tq[ipmt], ipmt );
      h2_q[nrun]->Fill( f_q[ipmt], ipmt );
      
      h_q[nrun][ipmt]->Fill( f_q[ipmt] );
      if ( f_tt[ipmt]>8 && f_tt[ipmt]<16 ) h_qsum[sn]->Fill( f_q[ipmt] );
      if ( f_q[ipmt]>200 ) h_q[nrun][ipmt]->Fill( f_q[ipmt] );
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
    h_q[nrun][ich]->Fit( fgaus[nrun][ich], "NQR" );

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

  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
  {
    int sn = ipmt/64;     // south or north
    int quad = ipmt/32;    // quadrant

    if ( verbose )
    {
        c_charge[quad]->cd( ipmt%32 + 1  );
        h_q[nrun][ipmt]->Draw();
        h_q[nrun][ipmt]->Rebin(10);
        gPad->SetLogy(1);

        c_time[quad]->cd( ipmt%32 + 1 );
        h_q[nrun][ipmt]->Draw();
        h_q[nrun][ipmt]->Rebin(10);
        //gPad->SetLogy(1);
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

  // Plot the charge at each radii
  // South
  std::map<float, int> bbc_r;
  bbc_r[7.51] = 0;
  bbc_r[8.52] = 1;
  bbc_r[9.84] = 2;
  bbc_r[10.24] = 3;
  bbc_r[11.36] = 4;
  bbc_r[12.38] = 5;
  bbc_r[13.01] = 6;

  int splots[7] = {1,1,1,1,1,1,1};

  ac[icvs] = new TCanvas("c_qbyr_south","south q by radius",1000,1000);
  ac[icvs]->Divide(4,2);
  for (int ipmt=0; ipmt<64; ipmt++)
  {
    float x = TubeLoc[ipmt][0];
    float y = TubeLoc[ipmt][1];
    float r = sqrt(x*x+y*y);
    r = round( r*100. )/100.;
    int ir = bbc_r[r];
    ac[icvs]->cd( ir + 1 );
    h_q[nrun][ipmt]->SetLineColor( splots[ir] );
    h_q[nrun][ipmt]->Rebin(10);
    if ( splots[ir] == 1 )
      h_q[nrun][ipmt]->Draw();
    else
      h_q[nrun][ipmt]->Draw("same");
    gPad->SetLogy(1);
    splots[ir]++;
  }
  name = dir + "qbyr_south.png";
  ac[icvs]->SaveAs( name );
  icvs++;

  int nplots[7] = {1,1,1,1,1,1,1};

  ac[icvs] = new TCanvas("c_qbyr_north","north q by radius",1000,1000);
  ac[icvs]->Divide(4,2);
  for (int ipmt=64; ipmt<128; ipmt++)
  {
    float x = -TubeLoc[ipmt%64][0];
    float y = TubeLoc[ipmt%64][1];
    float r = sqrt(x*x+y*y);
    r = round( r*100. )/100.;
    int ir = bbc_r[r];
    ac[icvs]->cd( ir + 1 );
    h_q[nrun][ipmt]->SetLineColor( nplots[ir] );
    h_q[nrun][ipmt]->Rebin(10);
    if ( nplots[ir] == 1 )
      h_q[nrun][ipmt]->Draw();
    else
      h_q[nrun][ipmt]->Draw("same");
    gPad->SetLogy(1);
    nplots[ir]++;
  }
  name = dir + "qbyr_north.png";
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
