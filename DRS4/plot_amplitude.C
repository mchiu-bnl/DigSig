// plot the amplitude distributions of the root file from drs4
int plot_amplitude(const char *fname = "test.root")
{
  int do_display = 0;
  TString name;
  TString title;
  TFile *tfile = new TFile(fname,"READ");

  Int_t evt = 0;
  const int NCH = 4;
  const int NSAMPLES = 1024;
  Double_t t[NCH][NSAMPLES];  // time
  Double_t ch[NCH][NSAMPLES]; // voltage
  TTree *rec = (TTree*)tfile->Get("rec");
  rec->SetBranchAddress("evt",&evt);
  rec->SetBranchAddress("t1",&t[0]);
  rec->SetBranchAddress("t2",&t[1]);
  rec->SetBranchAddress("t3",&t[2]);
  rec->SetBranchAddress("t4",&t[3]);
  rec->SetBranchAddress("ch1",&ch[0]);
  rec->SetBranchAddress("ch2",&ch[1]);
  rec->SetBranchAddress("ch3",&ch[2]);
  rec->SetBranchAddress("ch4",&ch[3]);

  TCanvas *c_display = 0;
  if ( do_display )
  {
    c_display = new TCanvas("drs4","drs4 display",800,800);
    c_display->Divide(1,4);
  }

  TCanvas *c_ampl = new TCanvas("c_ampl","DRS4 amplitudes",425,550);
  c_ampl->Divide(1,2);

  TGraph *gpulse[NCH] = {0};
  TH1 *h_ampl[NCH] = {0};
  TH1 *h_time[NCH] = {0};
  for (int ich=0; ich<NCH; ich++)
  {
    gpulse[ich] = new TGraph();
    name = "gch"; name += ich;
    gpulse[ich]->SetName(name);
    gpulse[ich]->SetMarkerStyle(20);
    gpulse[ich]->SetMarkerSize(0.4);
    gpulse[ich]->SetMarkerColor(ich+1);
    gpulse[ich]->SetLineColor(ich+1);

    name = "h_ampl"; name += ich;
    title = "ch"; title += ich;
    h_ampl[ich] = new TH1F(name,title,10000,0.,1000.);
    h_ampl[ich]->SetLineColor(ich+1);
    h_ampl[ich]->SetXTitle("ampl (mV)");

    name = "h_time"; name += ich;
    title = "ch"; title += ich;
    h_time[ich] = new TH1F(name,title,10000,0.,1000.);
    h_time[ich]->SetLineColor(ich+1);
    h_time[ich]->SetXTitle("time (ns)");
  }

  Long64_t nentries = rec->GetEntries();
  cout << "Found " << nentries << " events" << endl;
  //nentries = 100;
  for (int ievt=0; ievt<nentries; ievt++)
  {
    rec->GetEntry(ievt);
    //cout << "evt " << evt << endl;

    TSpline3 *spline[NCH] = {0};
    Double_t samp_vmax[NCH] = {-9999.};  // value of sampled maximum
    Double_t samp_tmax[NCH] = {-9999.}; // time of sample at maximum
    Double_t max_ampl[NCH] = {-9999.};    // amplitude at actual maximum
    Double_t max_time[NCH] = {-9999.};    // time at actual maximum

    // Read in data
    for (int ich=0; ich<NCH; ich++)
    {
      for (int isamp=0; isamp<NSAMPLES; isamp++) 
      {
        gpulse[ich]->SetPoint(isamp,t[ich][isamp],ch[ich][isamp]);
        if ( ch[ich][isamp]>samp_vmax[ich] )
        {
          samp_vmax[ich] = ch[ich][isamp];
          samp_tmax[ich] = t[ich][isamp];
        }
      }

      // Do a spline fit, and search around +/- 0.5 ns for peak
      name = "sp"; name += ich;
      spline[ich] = new TSpline3(name,gpulse[ich]);
      for (Double_t gtime=samp_tmax[ich]-0.5; gtime<samp_tmax[ich]+0.5; gtime+=0.01)
      {
        Double_t eval = spline[ich]->Eval(gtime);
        if ( eval > max_ampl[ich] )
        {
          max_ampl[ich] = eval;
          max_time[ich] = gtime;
        }
      }

      if (do_display)
      {
        c_display->cd(ich+1);
        gpulse[ich]->Draw("ap");
        spline[ich]->Draw("same");
        gPad->Modified();
        gPad->Update();
      }
    }

    if ( do_display )
    {
      for (int ich=0; ich<NCH; ich++)
      {
        cout << ich << "\t" << max_time[ich] << "\t" << max_ampl[ich] << endl;
      }

      string junk;
      cout << "? ";
      cin >> junk;
      if ( junk[0] == 'q' ) break;
    }

    for (int ich=0; ich<NCH; ich++)
    {
      h_ampl[ich]->Fill( max_ampl[ich] );
      h_time[ich]->Fill( max_time[ich] );
    }

    for (int ich=0; ich<NCH; ich++)
    {
      if ( spline[ich]!=0 ) delete spline[ich];
    }
  }

  c_ampl->cd(1);
  for (int ich=0; ich<NCH; ich++)
  {
    h_ampl[ich]->Rebin(10);
    if ( ich==0 ) h_ampl[ich]->Draw();
    else          h_ampl[ich]->Draw("same");
  }
  c_ampl->cd(2);
  for (int ich=0; ich<NCH; ich++)
  {
    h_time[ich]->Rebin(10);
    if ( ich==0 ) h_time[ich]->Draw();
    else          h_time[ich]->Draw("same");
  }

  return 1;
}

