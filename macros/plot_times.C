// Plot the time resolution for the ANL MCP-PMT
//

void plot_times(const char *flist = "runs.list")
{
  ifstream inflist(flist);

  TString rootfname;
  TString name;

  const int MAXFILES = 1000;
  TFile *tfile[MAXFILES];
  int nfile = 0;
  Double_t mean[MAXFILES];    // in ns
  Double_t rms[MAXFILES];     // in ns
  Double_t rmscorr[MAXFILES]; // in ps
  Double_t laser_rms_sqr = 0.019*0.019; // in ns^2

  // Get the timing resolution
  while ( inflist >> rootfname )
  {
    cout << rootfname << endl;
    name = rootfname;
    name.ReplaceAll(".root","_times.root");

    tfile[nfile] = new TFile(name,"READ");

    TTree *t = (TTree *)tfile[nfile]->Get("t");
    t->Draw("t0-t3>>h_tdiff","t0>100&&t0<200");
    TH1 *h_tdiff = tfile[nfile]->Get("h_tdiff");
    mean[nfile] = h_tdiff->GetMean();
    rms[nfile] = h_tdiff->GetRMS();
    rmscorr[nfile] = sqrt(rms[nfile]*rms[nfile] - laser_rms_sqr)*1000;  // convert to ps
    cout << mean[nfile] << "\t" << rms[nfile] << "\t" << rmscorr[nfile] << endl;

    nfile++;
  }

  // Now get the Npe
  ifstream inflist_npe(flist);
  int nperun = 0;
  int ch[MAXFILES];
  Double_t npe[MAXFILES];
  Double_t vdt[MAXFILES];
  Double_t vdtrms[MAXFILES];

  while ( inflist_npe >> rootfname )
  {
    name = rootfname;
    name.ReplaceAll(".root","_integ.txt");
    ifstream npefile(name);

    npefile >> ch[nperun] >> vdt[nperun] >> vdtrms[nperun];
    npe[nperun] = (vdt[nperun]/vdtrms[nperun])*(vdt[nperun]/vdtrms[nperun]);

    vdt[nperun] *= 1000./50.; // divide by 50 ohms and convert to pC
    vdtrms[nperun] *= 1000./50.; // divide by 50 ohms and convert to pC
    cout << vdt[nperun] << "\t" << vdtrms[nperun] << "\t" << npe[nperun] << endl;

    nperun++;
  }

  TGraph *g_tres = new TGraph(nperun,npe,rmscorr);
  g_tres->SetName("g_tres");
  g_tres->Draw("ap");
  g_tres->GetHistogram()->SetTitle("Time Resolution");
  g_tres->GetHistogram()->SetXTitle("N_{pe}");
  g_tres->GetHistogram()->SetYTitle("#sigma_{t} (ps)");

}
