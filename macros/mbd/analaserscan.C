//
// Plot laser amplitudes, make gain curves, extract HV to run gain at 
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
    name = "h_laseramp"; name += ich; name += "_"; name += nrun;
    title = name;
    h_laseramp[nrun][ich] = new TH1F(name,title,1610,-100,16000);
    //h_laseramp[nrun][ich] = new TH1F(name,title,160,0,16000);
  }

  Int_t   evt;
  Float_t t[MAXCH];  // time
  Float_t ch[MAXCH]; // voltage

  cout << "tfname " << tfname << endl;

  TFile *tfile = new TFile(tfname,"READ");
  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&evt);
  for (int ich=0; ich<NCH; ich++)
  {
    name = "t"; name += ich;
    tree->SetBranchAddress(name,&t[ich]);
    name = "ch"; name += ich;
    tree->SetBranchAddress(name,&ch[ich]);
  }

  int nentries = tree->GetEntries();
  for (int ientry=0; ientry<nentries; ientry++)
  {
    tree->GetEntry(ientry);

    for (int ich=0; ich<NCH; ich++)
    {
      //cout << evt << "\t" << t[1] << "\t" << ch[1] << "\t" << t[14] << "\t" << ch[14] << endl;
      h_laseramp[nrun][ich]->Fill( ch[ich] );
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

  if ( verbose && nrun==0 )
  {
    const int NCANVAS = 4;
    TCanvas *c_laseramp[NCANVAS];
    for (int icv = 0; icv<NCANVAS; icv++)
    {
      name = "c_laser"; name += icv;
      c_laseramp[icv] = new TCanvas(name,name,1600,800);
      c_laseramp[icv]->Divide(8,4);
    }

    for (int ich=0; ich<NCH; ich++)
    {
      int sn = ich/128;     // south or north
      int quad = ich/64;    // quadrant
      int ch = (ich/16)*8 + ich%8;
      int tq = (ich/8)%2;   // 0 = T-channel, 1 = Q-channel
      if ( tq==1 )  // charge channel
      {
        //c_laseramp[quad]->cd( ch%32 + 1 );
        //h_laseramp[nrun][ich]->Draw();
      }
      else          // time channel
      {
        c_laseramp[quad]->cd( ch%32 + 1 );
        //h_laseramp[nrun][ich]->Rebin(100);
        h_laseramp[nrun][ich]->Draw();
        /*
        gPad->SetLogy(1);
        gPad->Modified();
        gPad->Update();
        */
      }
    }

  }

}

void analaserscan(const char *fname = "hvscan.62880")
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
  ifstream scanfile(fname);
  for (int irun=0; irun<MAXRUNS; irun++)
  {
    scanfile >> run_number[irun] >> hvscale[irun];
    cout << run_number[irun] << "\t" << hvscale[irun] << endl;

    // get name of root file
    rootfname = "prdf_"; rootfname += run_number[irun]; rootfname += "_times.root";
    cout << "Processing " << rootfname << endl;

    anafile( rootfname, irun );
  }

  // Make the canvases for the gain curves
  const int NCANVAS = 4;
  TCanvas *c_laserscan[NCANVAS];
  for (int icv = 0; icv<NCANVAS; icv++)
  {
    name = "c_laserscan"; name += icv;
    c_laserscan[icv] = new TCanvas(name,name,1600,800);
    c_laserscan[icv]->Divide(8,4);
  }

  // Make and draw the gain curves, and
  // write out the HV scale needed to get XX gain
  ofstream gainfile("gains.out");
  float gain_wanted = 0.1;
  for (int ich=0; ich<NCH; ich++)
  {
    int sn = ich/128;   // south or north
    int quad = ich/64;  // quadrant
    int pmtch = (ich/16)*8 + ich%8;    // pmt channel

    // Create the TGraph with the summary results
    name = "g_laserscan"; name += ich;
    title = "ch "; name += pmtch;
    g_laserscan[ich] = new TGraphErrors(MAXRUNS,hvscale,laseramp[ich],0,laseramperr[ich]);
    g_laserscan[ich]->SetName(name);
    g_laserscan[ich]->SetTitle(title);
    g_laserscan[ich]->SetMarkerStyle(20);
    g_laserscan[ich]->SetMarkerSize(0.5);

    int tq = (ich/8)%2;   // 0 = T-channel, 1 = Q-channel

    // only look at charge channels
    if ( tq==1 )
    {
      //cout << "ch " << pmtch << endl;
      c_laserscan[quad]->cd( pmtch%32 + 1 );
      g_laserscan[ich]->Draw("acp");
      g_laserscan[ich]->GetHistogram()->SetTitleSize(4);

      // now step down until we find the gain we want
      double maxgain = g_laserscan[ich]->Eval(1.0);
      for (double iv=0.5; iv<1.0; iv+=0.001)
      {
        double ratio = g_laserscan[ich]->Eval(iv)/maxgain;
        if ( ratio>0.1 )
        {
          gainfile << pmtch << "\t" << iv << "\t" << ratio << endl;
          break;
        }
      }
    }
  }

  gainfile.close();
}

