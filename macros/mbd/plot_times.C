//
// Plot comparison of times with REFCH as the reference channel
//

// Set reference channel to compare to
// Should maybe figure a way to get 1st good ch to set as reference
//const int REFCH = 0;
//const int REFCH = 16;
const int REFCH = 32;
//const int REFCH = 64;

void plot_times(const char *tfname = "test_1_tp_times.root")
{
  const int MAXCH = 256;
  const int MAXBOARDS = MAXCH/16;

  // Get the number of actual channels to process
  int NCH = MAXCH;                 // total number of channels (including charge channels)
  int NBOARDS = MAXBOARDS;

  ifstream configfile("digsig.cfg");
  if ( configfile.is_open() )
  {
    string junk;
    configfile >> junk >> NCH;
    NBOARDS = NCH/16;

    cout << "Found config file digsig.cfg" << endl;
    cout << "Setting NCH = " << NCH << endl;
  }

  Int_t   evt;
  Float_t t[MAXCH];  // time
  Float_t ch[MAXCH]; // voltage

  TString name;
  TString title;
  TFile *tfile = new TFile(tfname,"READ");
  TTree *tree = (TTree*)tfile->Get("t");

  /*
  // More detailed analysis loop
  tree->SetBranchAddress("evt",&evt);
  for (int ich=0; ich<NCH; ich++)
  {
    name = "t"; name += ich;
    tree->SetBranchAddress(name,&t[ich]);
    name = "ch"; name += ich;
    tree->SetBranchAddress(name,&ch[ich]);
  }

  int nentries = tree->GetEntries();

  nentries = 125;
  for (int ientry=0; ientry<nentries; ientry++)
  {
    tree->GetEntry(ientry);

    //cout << evt << "\t" << t[1] << "\t" << ch[1] << "\t" << t[14] << "\t" << ch[14] << endl;
  }
  */

  // Quick and dirty drawing
  TCanvas *c_tdiffbyevt[MAXBOARDS];
  TCanvas *c_tdiff[MAXBOARDS];
  for (int iboard=0; iboard<NBOARDS; iboard++)
  {
    name = "c_tdiffbyevt"; name += iboard;
    title = "tpulse diff vs evt, bd"; title += iboard;
    c_tdiffbyevt[iboard] = new TCanvas(name,title,1000,1000);
    c_tdiffbyevt[iboard]->Divide(2,4);

    name = "c_tdiff"; name += iboard;
    title = "tpulse diff, bd"; title += iboard;
    c_tdiff[iboard] = new TCanvas(name,title,1000,1000);
    c_tdiff[iboard]->Divide(2,4);
  }

  TString cut;
  tree->SetMarkerStyle(20);
  tree->SetMarkerSize(0.1);
  for (int ich=0; ich<NCH; ich++)
  {
    // skip charge channels
    if ( (ich/8)%2 == 1 ) continue;

    int board = ich/16;
    int ch_in_board = ich%16;

    c_tdiffbyevt[board]->cd(ch_in_board+1);

    //name = "t0-t"; name += ich; name += ":evt";
    name = "t"; name += REFCH; name +="-t"; name += ich; name += ":t"; name += REFCH;
    //cut = "t"; cut += REFCH; cut += ">12&&t"; cut += ich; cut += ">-100";
    cut = "t"; cut += REFCH; cut += ">-200&&t"; cut += ich; cut += ">-200";
    tree->Draw(name,cut);

    c_tdiff[board]->cd(ch_in_board+1);

    name = "t"; name += REFCH; name += "-t"; name += ich;
    //cut = "t"; cut += REFCH; cut += ">12&&t"; cut += ich; cut += ">-100";
    cut = "t"; cut += REFCH; cut += ">-200&&t"; cut += ich; cut += ">-200";
    tree->Draw(name,cut);

  }

  // Now get the relative resolution for each pair combination
  TCanvas *c_tdiff_temp = new TCanvas("c_tdiff_temp","temp tdiff dist",400,400);
  TH1 *h_tdiff = new TH1F("h_tdiff","tdiff Mean distribution",150,0,-1);
  TH1 *h_tdiffrms = new TH1F("h_tdiffrms","tdiff RMS distribution",150,0,-1);
  TH1 *h_tres = new TH1F("h_tres","Timing Resol's, every pair",150,0,-1);
  TH1 *htemp_tdiff = new TH1F("htemp_tdiff","tdiff distribution",25000,-25,25);

  h_tdiff->SetXTitle("ns");
  h_tdiffrms->SetXTitle("ns");
  h_tres->SetXTitle("ps");

  for (int ich1=0; ich1<NCH-1; ich1++)
  {
    // skip charge channels
    if ( (ich1/8)%2 == 1 ) continue;

    for (int ich2=ich1+1; ich2<NCH; ich2++)
    {
      // skip charge channels
      if ( (ich2/8)%2 == 1 ) continue;

      htemp_tdiff->Reset();

      name = "t"; name += ich1; name += "-t"; name += ich2; name += ">>htemp_tdiff";
      cut = "t"; cut += ich1; cut += ">12&&t"; cut += ich1; cut += "<24";
      cut += "&&t"; cut += ich2; cut += ">12&&t"; cut += ich2; cut += "<24";
      tree->Draw(name,cut);

      Double_t mean = htemp_tdiff->GetMean(); 
      Double_t rms = htemp_tdiff->GetRMS(); 
      if ( rms==0 ) continue;

      // check against different boards
      if ( ich1>=64 ) continue;
      cout << name << "\t" << mean << "\t" << rms << endl;
      h_tdiff->Fill(mean);
      h_tdiffrms->Fill(rms);
      h_tres->Fill(1000.*rms/sqrt(2));  // sgl ch resolution, in ps
    }

  }
  TCanvas *c_tdiff2 = new TCanvas("c_tdiff_dist","tdiff dist",900,400);
  c_tdiff2->Divide(2,1);
  c_tdiff2->cd(1);
  h_tdiff->Draw();
  c_tdiff2->cd(2);
  h_tres->Draw();

  //gSystem->Sleep(30*1000);

  // Save canvases
  for (int iboard=0; iboard<NBOARDS; iboard++)
  {
    c_tdiffbyevt[iboard]->SaveAs(".png");
    c_tdiff[iboard]->SaveAs(".png");
    c_tdiff2->SaveAs(".png");
  }
}

