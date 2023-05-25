#include <stdlib.h>
#include <TPad.h>

int digsig_evtdisplay(const char *fname = "dt5742.root")
{
  TString name;
  TString title;
  TFile *tfile = new TFile(fname,"READ");

  Int_t evt;

  // MBD 1008
  const int NCH = 256;
  const int NSAMPLES = 31;

  /*
  // DRS4
  const int NCH = 4;
  const int NSAMPLES = 1024;
  */

  /*
  // CAEN DT5742
  const int NCH = 16;
  const int NSAMPLES = 1024;
  */

  /*
  // MBD DS TEST
  const int NCH = 64;
  const int NSAMPLES = 20;
  */

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

  //TCanvas *c_display = new TCanvas("c_display","event display",1600,800);
  TCanvas *c_display[4];
  for (int icv=0; icv<4; icv++)
  {
    name = "c_display"; name += icv;
    c_display[icv] = new TCanvas("c_display","event display",800,800);
    c_display[icv]->Divide(8,8);
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

  c_display[0]->cd(1);

  int nskipped = 0; // number of skipped, uninteresting events
  int plotevt = -1; // an event that was specified to plot
  int interactive = 1;  // whether to have interactive plotting

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
        if ( ich==0 && isamp<10 )
        {
          cout << "CH " << ich << "\t" << t[ich][isamp] << "\t" << ch[ich][isamp] << endl;
        }
        */

        /*
        if ( ich==4 && ch[ich][isamp]>1530. )
        {
          trig = 1;
        }
        */
      }

    }

    // if an event number was specified, plot it
    if ( plotevt > 0 )
    {
      if ( evt != plotevt ) continue;
      else
      {
        plotevt = -1;
      }
    }

    if (trig)
    {
      for (int ich=0; ich<NCH; ich++)
      {
        int cv = ich/64;
        c_display[cv]->cd(ich%64+1);

        //gpulse[ich]->GetXaxis()->SetRangeUser(165,200);
        //gpulse[ich]->GetXaxis()->SetRangeUser(15,200);
        //gpulse[ich]->GetXaxis()->SetRangeUser(285,300);
        gpulse[ich]->Draw("alp");
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
      if ( interactive )
      {
        cin >> junk;
        if ( junk[0] == 'q' )
        {
          break;
        }
        else if ( junk[0] == 'c' )
        {
          interactive = 0;
        }
        else if ( junk[0] == 'w' )
        {
          name = "evt_"; name += evt; name += ".png";
          c_display[cv]->SaveAs(name);
        }
        else if ( isdigit(junk[0]) )
        {
          plotevt = atoi(junk.c_str());
          cout << "Skipping to event " << plotevt << endl;
        }
      }
    }
    else
    {
      nskipped++;
    }

  }

  return 1;
}

