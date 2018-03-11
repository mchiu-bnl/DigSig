// plot the uniformity from the integrals in the dirlist
// dirlist is a text file with the list of the directories to look in
#include <fstream>
#include <iostream>

void plot_uniformity(const char *dirlistfname = "scan.list")
{
  ifstream dirlist(dirlistfname);
  TFile *tfile = 0;
  TH1 *h_integral = 0;

  //TH2F *h2_bbcpmt = new TH2F("h2_bbcpmt_laserscan","h2_bbcpmt_laserscan",9,0,9,16,0,16);
  //TH2F *h2_bbcpmt = new TH2F("h2_bbcpmt_laserscan","bbcpmt laserscan",9,139.5,148.5,16,118.5,134.5);
  //TH2F *h2_bbcpmt = new TH2F("h2_bbcpmt_laserscan","bbcpmt laserscan",8,139.5,147.5,11,123.5,134.5);
  TH2F *h2_bbcpmt = new TH2F("h2_bbcpmt_laserscan","bbcpmt laserscan",8,0.,20,11,0,19);
  h2_bbcpmt->SetXTitle("X (mm)");
  h2_bbcpmt->SetYTitle("Y (mm)");

  TString dirname;
  TString name;
  int x, y, z;
  while ( dirlist >> dirname )
  {
    //cout << dirname << endl;
    name = dirname; name += "/dt5742_integral.root";
    tfile = new TFile(name,"READ");
    h_integral = (TH1*)tfile->Get("h_integral0");

    Double_t integ = h_integral->GetMean();
    Double_t integ_err = h_integral->GetMeanError();
    Double_t integ_rms = h_integral->GetRMS();
    Double_t nevents = h_integral->GetEntries();

    dirname.ReplaceAll("_"," ");
    istringstream grid(dirname.Data());
    grid >> x >> y >> z;
    cout << x << "\t" << y << "\t" << nevents << "\t" << integ << "\t" << integ_err << "\t" << integ/nevents << "\t" << integ_rms << endl;

    //h2_bbcpmt->Fill(x-140,y-119,integ/nevents);
    //h2_bbcpmt->Fill(x,y,integ/nevents);
    h2_bbcpmt->SetBinContent(x-139,y-123,integ/nevents);
  }

  gStyle->SetOptStat(0);
  TCanvas *ac = new TCanvas("bbcpmt_laserscan1","bbcpmt_laserscan1",800,800);
  h2_bbcpmt->Draw("COLZ");
  TCanvas *bc = new TCanvas("bbcpmt_laserscan2","bbcpmt_laserscan2",800,800);
  h2_bbcpmt->Draw("SURF2");
}

