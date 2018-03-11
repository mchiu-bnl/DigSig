#include <TPad.h>

int evtdisplay(const char *fname = "test.root")
{
  TString name;
  TString title;
  TFile *tfile = new TFile(fname,"READ");

  Int_t evt;
  const int NCH = 4;
  const int NSAMPLES = 1024;
  Double_t t[NCH][NSAMPLES];  // time
  Double_t ch[NCH][NSAMPLES]; // voltage
  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&evt);
  tree->SetBranchAddress("t0",&t[0]);
  tree->SetBranchAddress("t1",&t[1]);
  tree->SetBranchAddress("t2",&t[2]);
  tree->SetBranchAddress("t3",&t[3]);
  tree->SetBranchAddress("ch0",&ch[0]);
  tree->SetBranchAddress("ch1",&ch[1]);
  tree->SetBranchAddress("ch2",&ch[2]);
  tree->SetBranchAddress("ch3",&ch[3]);

  TCanvas *c_display = new TCanvas("drs4","drs4 display",800,800);
  c_display->Divide(1,4);

  TGraph *gpulse[NCH];
  for (int ich=0; ich<NCH; ich++)
  {
    gpulse[ich] = new TGraph();
    name = "gch"; name += ich;
    gpulse[ich]->SetName(name);
    gpulse[ich]->SetMarkerStyle(20);
    gpulse[ich]->SetMarkerSize(0.4);
    gpulse[ich]->SetMarkerColor(ich+1);
    gpulse[ich]->SetLineColor(ich+1);
  }

  c_display->cd(1);

  int nskipped = 0; // number of skipped, uninteresting events

  Long64_t nentries = tree->GetEntries();
  cout << "Found " << nentries << " events" << endl;
  for (int ievt=0; ievt<nentries; ievt++)
  {
    //if ( ievt<1078 ) continue;
    tree->GetEntry(ievt);
    //cout << "evt " << evt << endl;

    // Read in data
    int trig = 1;
    for (int ich=0; ich<NCH; ich++)
    {
      for (int isamp=0; isamp<NSAMPLES; isamp++) 
      {
        gpulse[ich]->SetPoint(isamp,t[ich][isamp],ch[ich][isamp]);

        /*
        //if ( isamp>100 && isamp<NSAMPLES-100 && ch[ich][isamp]<-5.0 )
        if ( t[ich][isamp]>10. && t[ich][isamp]<25. && ch[ich][isamp]>0. )
        {
          trig = 1;
        }
        */
      }

    }

    if (trig)
    {
      for (int ich=0; ich<NCH; ich++)
      {
        c_display->cd(ich+1);
        gpulse[ich]->Draw("alp");
        //gpulse[ich]->GetXaxis()->SetRangeUser(165,200);
        gpulse[ich]->GetXaxis()->SetRangeUser(15,200);
      }
      if ( gPad!= 0 )
      {
        gPad->Modified();
        gPad->Update();
      }

      cout << "Skipped " << nskipped << " events" << endl;
      nskipped = 0;

      string junk;
      cout << evt << " ? ";
      cin >> junk;
      if ( junk[0] == 'q' ) break;
      else if ( junk[0] == 'w' )
      {
        c_display->SaveAs(".png");
      }
    }
    else
    {
      nskipped++;
    }

  }

  return 1;
}

