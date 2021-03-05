//
// Convert root digsig file to matlab
//
#include <stdlib.h>
#include <stdio.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TFile.h>
#include <TTree.h>
#include <iostream>

using namespace std;

typedef struct {
  long type;
  long mrows;
  long ncols;
  long imagf;
  long namelen;
} Fmatrix;

int digsig_tomat(const char *fname = "dt5742.root",const int evtstart = 0, int evtend = 0)
{
  TString name;
  TString title;
  TFile *tfile = new TFile(fname,"READ");

  Int_t evt;

  /* // DRS4
  const int NCH = 4;
  const int NSAMPLES = 1024;
  */

  // CAEN DT5742
  const int NCH = 16;
  const int NSAMPLES = 1024;

  /*
  // MBD DS TEST
  const int NCH = 64;
  const int NSAMPLES = 20;
  */

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

  // Now get times and amplitudes
  Float_t f_time[NCH];   // time
  Float_t f_ampl[NCH];   // voltage

  name = fname; name.ReplaceAll(".root","_times.root");
  TFile *timesfile = new TFile(name,"READ");
  TTree *timestree = (TTree*)timesfile->Get("t");
  for (int ich=0; ich<NCH; ich++)
  {
    name = "t"; name += ich;
    timestree->SetBranchAddress(name,&f_time[ich]);
    name = "ch"; name += ich;
    timestree->SetBranchAddress(name,&f_ampl[ich]);
  }

  //TCanvas *c_display = new TCanvas("c_display","event display",1600,800);
  TCanvas *c_display = new TCanvas("c_display","event display",800,800);
  c_display->Divide(4,4);

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

  c_display->cd(1);

  int nskipped = 0; // number of skipped, uninteresting events
  int plotevt = -1; // an event that was specified to plot
  int interactive = 1;  // whether to have interactive plotting

  Long64_t nentries = tree->GetEntries();
  if ( evtend == 0 )
  {
    evtend = nentries;
  }
  cout << "Found " << nentries << " events" << endl;
  cout << "Processing evts " << evtstart << " to " << evtend << endl;

  // We are only saving the region around where the peaks are
  int beg_samp = 240;  // beginning sample number of window
  int end_samp = 339;  // end sample number of window
 
  //*== Set up matlab v4 header
  const int NCOLS = 6; // [evt][ch][t][v][ftime][fvolt]

  // in matlab, the order is [mrows][ncol]
  //double *aa = new double[x.ncols][x.mrows]; // this doesn't work
  //double aa[4][1024*16*2]; // this does work, but can't dynamically set size
  double *aa[6];
  for (int icol=0; icol<NCOLS; icol++)
  {
    //aa[icol] = new double[1024*16*(evtend-evtstart)];
    aa[icol] = new double[100*1*(evtend-evtstart)]; // 100 samples, 1 channel to save
  }

  int ngoodevt = 0;
  int row = 0;
  for (int ievt=evtstart; ievt<evtend; ievt++)
  {
    tree->GetEntry(ievt);
    //cout << "evt " << evt << endl;
 
    timestree->GetEntry(ievt);
    //cout << ievt << "\t" << f_time[0] << "\t" << f_ampl[0] << endl;

    // Read in data
    for (int ich=0; ich<NCH; ich++)
    {
      // skip all but ch3 for now
      if (ich!=3) continue;

      // skip any event with bad times for ch3
      if ( f_time[ich] <-100 || f_time[ich]>200 ) continue;

      ++ngoodevt;

      for (int isamp=beg_samp; isamp<=end_samp; isamp++) 
      {
        aa[0][row] = ievt+1;
        aa[1][row] = ich;
        aa[2][row] = t[ich][isamp];
        aa[3][row] = ch[ich][isamp];
        aa[4][row] = f_time[ich];
        aa[5][row] = f_ampl[ich];

        ++row;
      }
    }
  }

  cout << "num good events " << ngoodevt << endl;

  //*== Set up matlab v4 header and write to mat file
  char *pname;
  double *pr;
  double *pi;
  Fmatrix x;
  int mn;
  FILE *fp;
  double real_data = 1.0;
  double imag_data = 2.0;
  fp = fopen("mymatfile.mat", "wb");
  if (fp != NULL) {
    pname = "x";
    x.type = 0000;
    //x.mrows = NCH*(end_samp-beg_samp+1)*nentries;
    x.mrows = 1*(end_samp-beg_samp+1)*ngoodevt;
    x.ncols = NCOLS;
    x.imagf = 0;
    x.namelen = 2;
    pr = &real_data;;
    pi = &imag_data;
    fwrite(&x, sizeof(Fmatrix), 1, fp);
    fwrite(pname, sizeof(char), x.namelen, fp);
  }

  // write to mat file
  for (int icol=0; icol<NCOLS; icol++)
  {
    fwrite(aa[icol], sizeof(double), x.mrows, fp);
  }

  fclose(fp);
  return 1;
}

